# Week 07：Linux epoll 多连接服务端与故障排查

本周项目在第六周 TCP 通信基础上，学习非阻塞 I/O、epoll 的 LT/ET 模式、单线程多连接事件循环、timerfd 定时任务、空闲连接清理，以及 GDB/core 排障。

最终练习形态是一个运行在 Linux 上的**单线程、多连接、非阻塞 TCP 接收服务端**：使用 epoll 等待网络和定时器事件，使用 RAII 管理文件描述符，并通过定时任务清理空闲连接。

> 范围说明：当前服务端接收并统计原始字节，不发送响应，也没有接入第六周的长度前缀消息协议。它是教学版本，不是完整的生产级 TCP 服务端。
>
> 验收说明：本文依据 Week7 Day1～Day7 的练习接口整理。本次生成文档时未能重新访问 WSL 项目，文中的测试数量按精简后的练习方案说明；实际源码、构建结果及 core 排障结果需要在本地核对。文末验收记录保留待填写，不预先宣称通过。

## 1. 学习目标

- 区分“socket 是否阻塞”和“epoll 如何通知”这两个不同问题。
- 用 `UniqueFd` 表达独占资源所有权，避免泄漏和重复关闭。
- 理解 LT 与 ET 在没有读完数据时的行为差异。
- 使用一个线程管理监听 socket、多个连接 socket 和 timerfd。
- 明确区分暂时无数据、收到数据、EOF 和真正的接收错误。
- 记录连接最后活动时间，定期关闭空闲连接。
- 使用 GDB 查看调用栈、变量和源代码，并保存、读取 core 文件。
- 形成“功能实现、关键测试、故障定位、文档记录”的闭环。

## 2. 每日学习内容

| 日期 | 主题 | 核心内容 |
| --- | --- | --- |
| Day1 | 非阻塞 I/O 与 RAII | `UniqueFd`、移动语义、`fcntl`、`TryReceive` |
| Day2 | epoll 与 LT | `epoll_create1`、`epoll_ctl`、`epoll_wait`、持续就绪通知 |
| Day3 | ET 与连续读取 | `EPOLLET`、部分读取对照、`DrainReadable`、数据与 EOF 同时出现 |
| Day4 | TCP 多连接服务端 | 非阻塞 listener、`accept4`、连接容器、事件分发和清理 |
| Day5 | 定时器接入 epoll | `timerfd_create`、`timerfd_settime`、到期计数、周期统计 |
| Day6 | 空闲连接清理 | `steady_clock`、`lastActive`、活动续期、超时清理 |
| Day7 | 故障排查与回归 | 可控越界异常、GDB 调用栈、core 文件、正常路径测试 |

## 3. 环境与目录

### 3.1 环境要求

- Linux；当前练习环境为 WSL Ubuntu。
- 支持 C++20 的编译器，例如 GCC。
- CMake 3.20 或更新版本。
- GDB，用于 Day7 排障。
- Python 3，可选，用于手动创建 TCP 客户端。

`epoll` 和 `timerfd` 是 Linux 接口。本项目不能不作修改就直接使用 Windows/MSVC 编译运行。

当前所有服务端逻辑由单线程执行，不需要因为使用 epoll 或 timerfd 而额外创建线程。

### 3.2 练习目录结构

```text
week07_epoll_server/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── io_utils.h
│   ├── epoll_poller.h
│   ├── tcp_server.h
│   └── timer_fd.h
├── src/
│   ├── io_utils.cpp
│   ├── epoll_poller.cpp
│   ├── main.cpp                  # Day1 非阻塞演示
│   ├── epoll_demo.cpp            # Day2 LT 演示
│   ├── tcp_server.cpp
│   ├── tcp_server_demo.cpp
│   ├── timer_fd.cpp
│   └── core_debug_demo.cpp       # 独立故障练习，不混入服务端
├── tests/
│   ├── io_utils_test.cpp
│   ├── epoll_poller_test.cpp
│   ├── epoll_et_test.cpp
│   ├── tcp_server_test.cpp       # 包含精简后的 Day6 综合测试
│   └── timer_fd_test.cpp
└── build/                       # 本地构建产物，不作为源码提交
```

若已经采用早期的独立 `idle_timeout_test.cpp` 方案，可以保留；精简方案只把一个关键空闲测试追加到 `tcp_server_test.cpp`，不要求另建测试目标。

## 4. 核心模块与职责

### 4.1 `UniqueFd`：资源所有权

`UniqueFd` 独占一个文件描述符：

- 默认对象保存 `-1`，不代表已经创建了内核资源。
- 接管一个已有 fd，不会复制或新建对应的内核资源。
- 禁止拷贝，支持移动；移动后原对象不再拥有该 fd。
- 析构或调用 `Close()` 时释放资源。
- `Get()` 返回编号，不转移所有权。
- `IsValid()` 检查内部编号是否有效，不是向内核查询资源是否仍然存在。

`UniqueFd` 不区分自己管理的是 socket、epoll 还是 timerfd；它只管理描述符生命周期。

关闭对象后，之前保存的整数副本仍然可能是原来的数字。整数变量不会自动变成 `-1`，而且该数字以后可能被内核分配给其他资源。

### 4.2 `TryReceive`：把一次接收结果分类

对非阻塞流式 socket 调用 `recv()`，将常见结果转换成明确的状态：

| 结果 | 状态或处理 | 含义 |
| --- | --- | --- |
| 返回值大于 0 | `kData` | 实际读取到数据 |
| 返回值等于 0 | `kPeerClosed` | 请求读取非零字节时，接收方向到达 EOF |
| `EAGAIN` / `EWOULDBLOCK` | `kWouldBlock` | 当前暂时没有数据，不等于断开 |
| `EINTR` | 重试 | 接收被信号中断 |
| 其他错误 | 抛出 `std::system_error` | 由调用层决定如何处理连接 |

正文中的零字节 `\0` 是普通数据，不是 EOF。EOF 通过 `recv()` 的返回值判断。

### 4.3 `DrainReadable`：连续读取

反复调用 `TryReceive()`，直到暂时无数据或 EOF：

- 多次读到的数据追加到同一个结果中。
- `data` 非空与 `peerClosed == true` 可以同时成立。
- 必须先处理已经读到的数据，再决定是否关闭连接。
- 本练习默认限制一次接收批次最多累计 64 KiB，超限抛异常并放弃该连接。

这个限制是**接收批次保护**，不是应用消息长度限制，也不是完整的背压方案。超限前可能已经从 socket 取走部分数据，不能把异常理解为“读取操作自动回滚”。

一次 `DrainReadable()` 的返回结果可能包含一条消息的一部分，也可能包含多条消息。TCP 没有自动保留应用层消息边界。

### 4.4 `EpollPoller`：等待事件，不拥有连接

| 方法 | 作用 |
| --- | --- |
| 构造函数 | 创建 epoll 实例 |
| `Add(fd, events)` | 注册对某个 fd 的关注 |
| `Remove(fd)` | 取消关注，但不关闭目标 fd |
| `Wait(timeoutMs)` | 返回一批已经就绪的事件 |

`Wait()` 的超时参数：

- `0`：立即检查，不等待。
- 正数：没有事件时最多等待相应毫秒数，实际时间受调度影响。
- `-1`：没有事件就持续等待，直到事件或中断等情况发生。

返回事件数组的容量限制的是单次取得的事件数量，不是整个 epoll 能注册的连接数量。

当前练习版 `Wait()` 对系统调用失败直接抛异常，没有完整实现 `EINTR` 后保持原等待截止时间的恢复策略。

### 4.5 `TimerFd`：定时事件源

| 方法 | 作用 |
| --- | --- |
| 构造函数 | 创建非阻塞 timerfd，但不启动计时 |
| `Start(firstDelayMs, intervalMs)` | 设置第一次到期时间及后续间隔 |
| `Stop()` | 停止计时，保留 fd，可以再次启动 |
| `Consume()` | 读取并消费累计到期次数 |

定时器使用 `CLOCK_MONOTONIC`，适合相对时间间隔。设置使用 `itimerspec`：

- `it_value`：第一次到期时间；本项目使用相对时间。
- `it_interval`：后续重复间隔；全零表示不重复。
- 其中的 `timespec` 使用秒和纳秒，而不是 `timeval` 的秒和微秒。

```cpp
// 将剩余毫秒换算为纳秒；L 表示 long 类型整数常量。
result.tv_sec = milliseconds / 1000;
result.tv_nsec = (milliseconds % 1000) * 1'000'000L;
```

timerfd 使用 `read()`，不能交给使用 `recv()` 的接收函数。成功读取时得到一个 8 字节无符号整数：

- `read()` 返回值表示字节数，本例为 8。
- 写入 `expirations` 的内容表示到期次数，可能大于 1。
- 非阻塞读取没有待消费次数时返回 `-1` 并设置 `EAGAIN`，由 `Consume()` 转换为返回 0。

读取计数不会停止周期定时器。若没有消费到期次数，LT 模式下的 timerfd 会持续保持可读，可能导致事件循环反复立即返回。

### 4.6 `TcpServer`：单线程事件分发

对外接口包括：

```cpp
// port：0 表示系统分配端口。
// statisticsIntervalMs：定时统计和空闲检查周期。
// idleTimeoutMs：空闲超时；0 表示禁用空闲清理。
explicit TcpServer(
    std::uint16_t port = 0,
    int statisticsIntervalMs = 1000,
    int idleTimeoutMs = 5000);

// 只处理一轮事件，外层循环负责持续运行。
void RunOnce(int timeoutMs);
```

可观察指标：`GetPort()`、`ConnectionCount()`、`AcceptedCount()`、`ClosedCount()`、`ReceivedBytes()`、`TimerTicks()`、`IdleClosedCount()`。

这些成员在当前设计中只由同一个线程访问，因此没有使用 `std::atomic`。这不代表以后把 getter 放到其他线程中调用仍然自动安全。

## 5. fd 的创建、所有权与事件含义

进入 demo 主循环前，服务端新建三个描述符：

| 对象 | 创建接口 | 负责释放的成员 |
| --- | --- | --- |
| epoll 实例 | `epoll_create1()` | `poller_` 内部的 `UniqueFd` |
| 监听 socket | `socket()` | `listener_` |
| 定时器 | `timerfd_create()` | `statisticsTimer_` 内部的 `UniqueFd` |

每次 `accept4()` 成功，再创建一个连接 fd，由 `connections_` 中的 `Connection::socket` 管理。

fd 编号由内核分配。例如 3、4、5、6 只是可能出现的编号，不能写进业务逻辑。`bind()`、`listen()`、epoll 注册和定时器启动都不是创建新 fd 的操作。

| 被关注的对象 | `EPOLLIN` 在本例中的含义 | 对应处理 |
| --- | --- | --- |
| 监听 socket | 可以尝试接受连接 | `accept4()` |
| 连接 socket | 可以尝试读取数据或 EOF | `DrainReadable()` / `recv()` |
| timerfd | 存在未消费的到期次数 | `Consume()` / `read()` |

Linux 上普通 `accept()` 返回的连接不会自动继承监听 socket 的 `O_NONBLOCK`。本项目通过 `accept4(..., SOCK_NONBLOCK | SOCK_CLOEXEC)` 显式设置新连接属性。

监听 socket 自身也必须是非阻塞的，否则接受完现有连接后继续调用 `accept4()`，可能阻塞整个线程。

## 6. 阻塞、非阻塞、LT 与 ET

它们分属两个维度：

| 维度 | 要回答的问题 |
| --- | --- |
| 阻塞 / 非阻塞 | 操作当前不能完成时，调用是否等待？ |
| LT / ET | 就绪状态出现或持续时，epoll 如何通知？ |

### LT：水平触发

只要关注的就绪条件仍然成立，后续等待仍可能取得通知。例如发送 `AB` 后只读取 `A`，缓冲区仍有 `B`，LT 可以继续报告可读。

### ET：边沿触发

不能依赖“数据还没读完”就不断得到重复通知。因此通常配合非阻塞 I/O，一次通知后持续读取到 `EAGAIN`，或维护明确的继续处理机制。

Day3 用受控的 `socketpair` 实验对照 LT/ET；最终 `TcpServer` 采用 **LT + 非阻塞**。学习了 ET 不意味着服务端必须改用 ET。

非阻塞 socket 不等于 `epoll_wait()` 也不能等待。让线程在没有事件时等待，正是避免空转的重要方式。

## 7. 服务端运行过程

### 7.1 初始化

1. 创建 epoll、监听 socket 和 timerfd。
2. `bind()` 绑定回环地址与端口。
3. `listen()` 使 socket 进入监听状态。
4. 查询实际端口，将监听 socket 和 timerfd 加入 epoll。
5. `TimerFd::Start()` 内部调用 `timerfd_settime()`，启动周期计时。
6. 返回 `main()`，打印地址并进入事件循环。

### 7.2 每轮事件处理顺序

1. `epoll_wait()` 返回一批事件。
2. 处理本批已有连接的网络事件；监听和定时事件先做标记。
3. 若定时器就绪，消费计数、检查空闲连接并打印统计。
4. 若 listener 就绪，接受并注册新连接。
5. `RunOnce()` 返回，外层循环开始下一轮等待。

网络和定时事件可能同时出现，没有固定的内核返回顺序。程序通过标记延迟处理，明确实现“本批已有连接优先、定时维护其次、新连接最后”的顺序。

先读取本批数据可以刷新活动时间，减少同批网络就绪和超时检查相遇时的误清理。最后接受新连接则避免旧连接 fd 被复用后，与本批旧事件发生混淆。

这是单线程顺序调用，不存在 timerfd 自动插入执行 `HandleTimer()` 的情况。线程忙时，网络数据和定时器次数可以在内核中累积，等待程序后续处理。

## 8. 连接关闭与空闲清理

### 8.1 正常接收与结束发送

- 读到数据：统计字节，刷新 `lastActive`。
- 读到 `EAGAIN`：保留连接，等待后续事件。
- 读到 EOF：先处理本批剩余数据，再按本项目策略关闭连接。
- 接收错误或批次超限：本项目关闭对应连接。

`EPOLLRDHUP` 表示对端关闭连接或结束发送方向，不代表对端一定不能接收响应。`EPOLLHUP` 发生时也可能存在尚未读取的数据，因此不能一见通知就无条件丢弃缓冲区内容。

当前项目只接收、不回包，因此读到 EOF 后关闭整个连接。若以后加入请求响应，可能需要先发送完响应，再关闭连接。

### 8.2 两步释放连接

```cpp
// 取消 epoll 关注关系，不负责关闭 socket。
poller_.Remove(fd);

// 删除 Connection，析构其 UniqueFd 成员并关闭 socket。
connections_.erase(it);
```

`erase()` 删除指定连接；`clear()` 删除所有连接。一个客户端退出时，不能用 `clear()` 连带关闭其他客户端。

### 8.3 空闲时间设计

每条连接保存：

```cpp
using Clock = std::chrono::steady_clock;

struct Connection {
    UniqueFd socket;
    Clock::time_point lastActive;
};
```

- `using Clock` 只是类型别名，不创建时钟对象、线程或 fd。
- 接受连接时记录当前时间。
- 实际成功读到非空数据时覆盖旧时间。
- 定时检查时，计算 `now - lastActive`。
- 达到空闲阈值后，先收集 fd，再统一删除，避免遍历时删除当前元素导致迭代器失效。

活动时间采用服务端读取时刻，不是客户端发送时刻或网络包到达内核的时刻。处理本批事件优先并不保证内核中所有尚未返回的事件都已被处理。

默认每秒检查一次，空闲阈值 5 秒。连接在达到阈值后的某次检查中被关闭，不保证精确在第 5000 毫秒关闭。

### 8.4 指标含义

| 指标 | 含义 |
| --- | --- |
| `active` | 当前连接容器中的连接数 |
| `accepted` | 成功接管并注册的连接累计数 |
| `closed` | 通过 `CloseClient()` 关闭的连接累计数 |
| `idleClosed` | 上述关闭中，原因为空闲超时的数量 |
| `bytes` | 成功返回并被业务统计的接收字节数 |
| `ticks` | 已消费的累计定时器到期次数 |

`ticks` 不是日志行数；一次读取可能取得多个到期次数。`closed` 也不是析构阶段自动释放资源的计数，因此不能把所有资源释放都理解为一定产生一条关闭日志。

## 9. 构建与运行

以下命令均在 `week07_epoll_server` 项目目录执行。

> 如果需要分析已有 core，先保留与它匹配的可执行文件和源码，再重新编译。重新构建可能覆盖故障发生时的程序。

### 9.1 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build
```

### 9.2 运行基础演示

```bash
./build/nonblocking_io_demo
./build/epoll_demo
./build/epoll_et_test
```

Day1/Day3 的局部通信实验使用 `socketpair`，不是对外监听的 TCP 服务。Day4 之后的 `tcp_server_demo` 才使用真实回环 TCP 连接。

### 9.3 运行 TCP 服务端

```bash
./build/tcp_server_demo
```

默认配置相当于：

```cpp
// 自动分配端口，每秒执行统计和清理，空闲阈值为 5 秒。
net::TcpServer server(0, 1000, 5000);

for (;;) {
    // 定时器和网络事件都能唤醒等待中的线程。
    server.RunOnce(-1);
}
```

连接全部退出后，服务端仍继续监听和统计。当前 demo 使用默认 `Ctrl+C` 终止方式，未实现优雅停止；不能假定这种终止一定执行 C++ 析构函数，进程资源最终由操作系统回收。

## 10. 手动验证

### 10.1 多连接互不影响

在另一个终端执行下面的 Python 示例，把端口改为服务端实际输出的值：

```python
import socket

port = 39123  # 替换为实际监听端口。

a = socket.create_connection(("127.0.0.1", port), timeout=3)
b = socket.create_connection(("127.0.0.1", port), timeout=3)

a.sendall(b"hello from A")
b.sendall(b"hello from B")
a.close()

# A 退出后，B 仍应能发送。
b.sendall(b"B is still connected")
b.close()
```

观察：接受两个连接、收到两端数据、A 清理后 B 仍能继续发送。不同连接的日志顺序、fd 编号和单批字节数量不要求固定。

### 10.2 空闲连接被服务端关闭

```python
import socket

port = 39123  # 替换为实际监听端口。

with socket.create_connection(("127.0.0.1", port), timeout=3) as client:
    client.settimeout(10)
    # 不发送数据，等待服务端空闲清理。
    print(client.recv(1))  # 正常观察结果为 b''，表示 EOF。
```

服务端应出现 `[idle timeout]`、`[close]` 和相应统计变化。这里是服务端主动关闭，不是客户端主动结束发送。

## 11. 自动化测试

```bash
ctest --test-dir build --output-on-failure
```

按“Day6 只追加一个关键综合用例”的精简方案，Day7 完成后的注册目标为：

| 测试目标 | 主要验证内容 |
| --- | --- |
| `io_utils_test` | RAII、移动、非阻塞标志、接收结果分类 |
| `epoll_poller_test` | epoll 生命周期、LT、移除/重加、多 socket |
| `epoll_et_test` | LT/ET 部分读取对照、连续读取、EOF、批次限制 |
| `tcp_server_test` | 多连接、数据与 EOF、定时器集成、空闲与活跃连接隔离 |
| `timer_fd_test` | 初始状态、单次/周期到期、停止重启、参数校验 |
| `core_debug_normal_test` | 独立故障程序的正常路径 |

若保留独立 `idle_timeout_test`，测试目标数量会多一个。以实际 CMake 注册内容为准。一个 CTest 目标可以包含多个 C++ 测试函数，目标数量不等于场景数量。

### Day6 关键综合测试

`TestIdleTimeoutKeepsActiveClient()` 验证：

1. 两个客户端都成功连接。
2. A 周期性发送数据，持续刷新活动时间。
3. B 不发送数据，由服务端超时清理。
4. A 仍可正常工作，最后按 EOF 路径退出。
5. 正常 EOF 不额外计入 `IdleClosedCount()`。

为控制每日练习负荷，禁用清理、非法参数等额外边界场景可以后续补充，不作为这份精简方案已经验证的内容。

`PumpUntil()` 在同一个线程中反复执行 `RunOnce()`，直到 lambda 条件成立或测试截止时间到达。它没有创建后台线程。测试中的短周期只是加快验证，不能据此要求毫秒级精确调度。

## 12. Day7：GDB 与 core 排障

### 12.1 故障程序的范围

`core_debug_demo` 是独立程序，不属于服务端执行路径。

- 无参数：正常处理模拟 fd 列表 `{6, 7}`。
- `--crash`：故意把遍历上界多算一次，访问下标 2。
- `vector::at()` 检查越界并抛出 `std::out_of_range`。
- 程序故意不捕获该异常，通常通过默认终止路径产生 `SIGABRT`。

模拟 fd 数字不是实际打开的 socket。这是可控的逻辑越界异常练习，不是通过未定义行为制造的随机内存破坏。

### 12.2 正常运行

```bash
./build/core_debug_demo
```

预期包含：

```text
[event] index=0 fd=6
[event] index=1 fd=7
Normal path passed
```

自动化测试只执行正常模式，不向普通 CTest 用例传入 `--crash`。

### 12.3 调试异常抛出位置

```bash
gdb --args ./build/core_debug_demo --crash
```

在 GDB 中执行：

```gdb
catch throw
run
bt
```

在调用栈中找到 `HandleReadyEvent`，使用它的实际编号选择栈帧：

```gdb
# N 替换为 bt 显示的实际编号，不固定为某个数字。
frame N
info args
print index
print readyFds
list
```

关键证据应为：容器包含两个元素，正在访问的 `index` 却为 2。合法下标应为 0、1。

继续向调用者查看：

```gdb
up
info locals
list
```

根因位于 `ProcessBatch()` 的故障注入逻辑：遍历上界被多加了 1。不要只停留在“标准库抛异常了”，还要找到错误下标的来源。

### 12.4 保存并重新读取 core

在异常抛出断点检查完后，继续执行：

```gdb
continue
```

当 GDB 因 `SIGABRT` 停住进程时：

```gdb
generate-core-file build/week07-debug.core
quit
```

退出时若询问是否结束被调试进程，确认即可。若同名文件已存在且需要保留，请使用新的文件名。

重新读取：

```bash
gdb ./build/core_debug_demo ./build/week07-debug.core
```

```gdb
bt
```

core 是快照，不是仍在运行的进程，不能像活进程一样继续执行。异常终止阶段的调用栈受运行库实现影响；异常刚抛出时的 `catch throw` 检查可以更直接地观察原始参数。

必须使用匹配的可执行文件、调试信息和源码。core 可能包含进程内存中的敏感信息，不要随意上传；构建产物与 core 不应作为普通源码提交。

### 12.5 资源限制与自动 core 配置

```bash
# 当前 shell 的打开文件描述符数量软限制、硬限制。
ulimit -Sn
ulimit -Hn

# 当前 shell 的 core 大小限制。
ulimit -c

# 内核自动 core 命名或转交规则。
cat /proc/sys/kernel/core_pattern
```

fd 数量限制与 core 大小限制是不同概念。崩溃后没有在当前目录看到 core，不一定说明程序没有异常，可能是大小限制或系统服务接管了转储。

本练习不要求修改系统全局 `core_pattern`；通过 GDB 显式生成快照即可完成 core 读取练习。

### 12.6 排障记录模板

| 项目 | 应记录的内容 | 本地记录 |
| --- | --- | --- |
| 故障入口 | 使用的程序、参数和构建版本 | 待填写 |
| 故障现象 | 异常类型与终止信号 | 待填写 |
| 定位位置 | 源文件、函数、实际行号 | 待填写 |
| 关键变量 | `index`、容器元素数量、遍历上界 | 待填写 |
| 根因 | 遍历边界多算一次，访问非法下标 | 待本地证据确认 |
| 修正原则 | 正常遍历下标严格小于容器大小 | 待填写修正或对照结果 |
| core 证据 | 文件路径、匹配程序、读取结果 | 待填写 |
| 回归结果 | 正常模式和完整 CTest 输出 | 待填写 |

## 13. 关键设计取舍与当前限制

- **单线程**：实现简单，但慢操作或大量日志可能推迟其他连接和定时任务的处理。
- **最终服务端使用 LT**：允许接受连接时使用每轮预算；不能把这套预算逻辑原样理解为 ET 下也一定能再次获得通知。
- **接受连接预算为 32 次尝试**：它不是连接总数上限，也不是 `listen` 的 backlog。
- **只接收不发送**：尚未实现输出缓冲区、部分发送恢复、`EPOLLOUT` 注册切换和发送背压。
- **没有消息解析**：接收批次不是消息，第六周的完整协议尚未接入当前事件循环。
- **空闲检查为全表扫描**：每次时间复杂度为 O(N)，适合当前练习规模；未实现时间堆或时间轮。
- **每批读取上限是简单保护**：不是完整公平调度，也不保证单轮处理时间严格有界。
- **未限制总连接数**：没有完整实现 fd 耗尽、资源不足和所有暂态 accept 错误的恢复策略。
- **没有优雅停止**：暂未使用信号事件或专用唤醒机制协调退出。
- **异常隔离有边界**：部分接收错误仅关闭对应连接；epoll 操作等未处理异常仍可能传播到 `main()` 并结束程序。
- **没有跨线程接口保证**：如果未来增加并发访问，需要重新设计同步和所有权。

本周重点是掌握资源、就绪事件、连接状态和时间检查的关系，不应把教学版本等同于生产级服务。

## 14. 验收清单

以下项目需要根据本地结果填写，未勾选不表示失败，而是本次文档生成未重新验证。

- [ ] 编译通过，无新增编译告警。
- [ ] 全部已注册 CTest 目标通过。
- [ ] LT/ET 对照实验能解释“还有数据但是否再次通知”的区别。
- [ ] 两个客户端可以同时连接，一个退出不影响另一个。
- [ ] 没有客户端时，定时器仍能产生统计事件。
- [ ] 活跃连接可以续期，空闲连接能被清理。
- [ ] `core_debug_demo` 正常模式通过。
- [ ] 使用 GDB 定位了错误下标及其产生位置。
- [ ] 保存并重新读取 core，确认程序匹配。
- [ ] 补齐排障记录，保留必要的本地输出证据。

## 15. 本周复习要点

1. fd 是编号，RAII 对象表达所有权，epoll 注册关系不等于资源所有权。
2. 内核自动感知事件，但不会自动调用你的 C++ 处理函数，事件循环仍然必需。
3. 非阻塞描述调用行为，LT/ET 描述通知行为，不能混为一谈。
4. `EAGAIN` 表示暂时不能继续，不应直接当作 EOF。
5. 关闭通知可能与剩余数据同时存在，先读取再按业务策略处理关闭。
6. timerfd 决定何时检查，`lastActive` 与空闲阈值决定是否清理。
7. `read()` 返回的字节数与 timerfd 写入的到期次数是不同的值。
8. 计时单位的表示精度不等于线程实际执行的时间精度。
9. 排障要从现象追到调用栈、变量和根因，再用正常路径与回归测试验证。
10. 构建通过、演示正常、测试通过和性能达标是不同层面的证据。

## 16. 参考资料

- [epoll：就绪通知与 LT/ET](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [epoll_ctl：注册与事件标志](https://man7.org/linux/man-pages/man2/epoll_ctl.2.html)
- [accept / accept4](https://man7.org/linux/man-pages/man2/accept.2.html)
- [timerfd：创建、配置与读取](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)
- [GDB：调用栈](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Backtrace.html)
- [GDB：生成 core 文件](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Core-File-Generation.html)
- [Linux core 转储](https://man7.org/linux/man-pages/man5/core.5.html)
