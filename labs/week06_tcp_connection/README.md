# 第六周：TCP 连接层 v0.1

基于 C++20 和 Linux Socket API 的学习项目，实现 TCP 连接建立、长度前缀消息收发、接收超时、应用层状态管理与异常关闭，为后续非阻塞和 epoll 练习打基础。

当前演示在同一进程、同一线程中创建监听端、客户端和服务端连接，使用 IPv4 回环地址 `127.0.0.1`。`GET_PORT Ethernet0` 及其响应是固定演示字符串，不查询真实网络设备，也没有实现命令解析器。

## 1. 环境与运行

要求：Linux（当前在 WSL Ubuntu-24.04 验证）、支持 C++20 的编译器、CMake 3.20 或以上，以及 Make 或 Ninja。

在 WSL 中执行：

```bash
cd /home/xinhuo/code/cpp-labs/labs/week06_tcp_connection
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/tcp_connection_demo
./build/socket_timeout_test
./build/connection_lifecycle_test
```

CMake 使用 `include(CTest)`，通过 `BUILD_TESTING` 控制测试目标；启用 `-Wall -Wextra -Wpedantic`。核心库目标为 `tcp_connection_core`，演示目标为 `tcp_connection_demo`。

## 2. 项目组成与本周练习

| 文件（相对于本目录） | 职责 |
| --- | --- |
| `include/tcp_socket.h`、`src/tcp_socket.cpp` | socket 所有权、连接建立、原始字节收发、超时与状态 |
| `include/message_protocol.h`、`src/message_protocol.cpp` | 长度头编码、解码、长度校验和消息接收 |
| `src/main.cpp` | 固定请求响应演示与状态输出 |
| `tests/tcp_socket_test.cpp` | 基础接口、资源生命周期与字节收发测试 |
| `tests/message_protocol_test.cpp` | 完整消息、连续帧、非法长度和截断测试 |
| `tests/socket_timeout_test.cpp` | 超时参数、正常通信和停滞接收测试 |
| `tests/connection_lifecycle_test.cpp` | 状态、异常关闭、禁止复用与日志 |
| `CMakeLists.txt` | 构建目标与四个 CTest 测试项目 |

| 阶段 | 内容 |
| --- | --- |
| Day1 | RAII、描述符、禁止复制、析构释放 |
| Day2 | `bind`、`listen`、端口 0 自动分配、地址查询 |
| Day3 | `connect`、`accept`、监听对象和连接对象的区别 |
| Day4 | 部分收发循环、`EINTR` 重试、提前 EOF |
| Day5 | 长度前缀协议、网络字节序、二进制正文与边界测试 |
| Day6 | `SO_RCVTIMEO`、超时错误码、耗时测量 |
| Day7 | 应用层状态、异常关闭、日志与整周回归 |

## 3. 资源所有权与连接建立

`TcpSocket` 独占一个描述符。默认构造调用 `socket(AF_INET, SOCK_STREAM, 0)` 创建阻塞式 TCP socket；复制构造、复制赋值被删除，避免重复关闭。当前没有定义移动操作。

`Accept()` 返回 `TcpSocket{acceptedFd}`，私有构造函数接管 `accept()` 返回的描述符，不会再创建一个 socket。返回值使用 C++17 起保证的复制消除，不要求复制或移动对象。

演示流程：监听对象绑定 `127.0.0.1:0` 并监听 → 客户端连接分配到的服务端端口 → 监听对象调用 `Accept()` → 请求响应 → 关闭连接。握手由内核处理，所以当前单线程小消息演示可先连接再接受连接，但不能把这个顺序推广为任意规模的阻塞收发方案。

| 对象 | 本地端口 | 描述符 |
| --- | --- | --- |
| `listener` | 服务端监听端口 | 独立描述符 |
| `client` | 客户端临时端口 | 独立描述符 |
| `connection` | 与监听对象相同的服务端端口 | 不同于监听和客户端的描述符 |

描述符是进程内资源编号，不是端口号，关闭后号码可复用。关闭已接受连接不影响监听对象。

`Close()` 主动关闭，析构函数调用它兜底。关闭时保存旧描述符，将成员 `fd_` 置为 `-1`，再关闭旧描述符。重复调用 `Close()` 或 `Abort()` 不会重复关闭，也不覆盖已有结束原因。清理函数不抛异常；当前不报告 `close()` 返回的错误，也不盲目重试关闭。

## 4. 消息格式

TCP 是字节流，不保证一次发送对应一次接收。本项目以长度前缀确定应用层消息边界。

| 字段 | 大小 | 定义 |
| --- | --- | --- |
| 长度头 | 4 字节 | `std::uint32_t`，网络字节序（大端），仅表示正文长度 |
| 正文 | 0～1024 字节 | 原始字节，允许包含 `\0` |

上限由 `protocol::kMaxMessageSize` 定义。正文不额外发送字符串终止符。空正文合法，仍须发送四个零字节作为消息头。

正文 `PING` 的线上字节（十六进制）：

```text
00 00 00 04  50 49 4E 47
长度头        P  I  N  G
```

- 发送：先检查上限，再用 `htonl()` 转换长度，发送四字节消息头和正文。
- 接收：读取四字节头，用 `memcpy()` 复制到整数对象，用 `ntohl()` 转换并检查上限，再读取正文。
- `memcpy()` 仅复制字节，不转换字节序。消息头的 `std::string_view` 借用长度整数内存，在发送完成前该整数保持存活。
- `SendAll()` 直接发送字节；`SendMessage()` 自动增加长度头。正常正文 `std::string("A\0B", 3)` 与直接发送两个零字节制造半个消息头，是不同测试场景。

本地超长正文在写入任何字节前被拒绝，抛出 `std::length_error`，不关闭原连接。接收非法长度头则关闭连接，记录 `FAILED`，不尝试跳过未知数据继续解析。

## 5. 部分收发与超时

`SendAll()` 和 `ReceiveExact()` 根据系统调用实际返回的字节数累计进度，继续处理剩余字节。接收字符串预先构造成目标长度，作为可写缓冲区，而不是仅预留容量。

| 条件 | 当前处理 |
| --- | --- |
| 收发返回正数 | 累计进度 |
| 失败且 `errno == EINTR` | 重试 |
| 发送无进展或其他发送错误 | 关闭并记录 `FAILED`，抛异常 |
| 非零长度的接收请求得到 `recv == 0` | 观察到 EOF，关闭本端并记录 `PEER_CLOSED`，抛异常 |
| 接收超时，`EAGAIN/EWOULDBLOCK` | 关闭并记录 `TIMED_OUT`，抛 `std::system_error` |
| 其他接收错误 | 关闭并记录 `FAILED`，抛异常 |

发送使用 `MSG_NOSIGNAL`，避免因本次发送触发 `SIGPIPE` 而直接终止进程，错误仍通过返回值报告。

`SetReceiveTimeout(int timeoutMs)` 要求正整数毫秒，拆为 `timeval` 的秒和微秒，通过 `setsockopt(SOL_SOCKET, SO_RCVTIMEO, ...)` 设置。演示中双方连接设置 1000ms，停滞测试设置 200ms。未配置的 socket 保持系统默认等待行为。

三种时间概念不能混淆：

1. 接收超时限制一次接收调用的阻塞等待，不是整条消息的总期限；多次调用、零星数据和重试可使总耗时更长。
2. CTest 为每个测试项目配置 10 秒兜底超时，用于阻止测试挂死，不代替程序的异常处理。
3. `elapsed` 用 `steady_clock` 测量实际耗时；测试不要求精确等于 200ms，仅检查不应早于 100ms 返回。

当前是在阻塞模式和接收超时配置下解释 `EAGAIN/EWOULDBLOCK`。非阻塞模式中它也可能只是当前无数据，不能照搬为超时错误。

`ReceiveMessage()` 必须返回完整消息。读取下一条消息时，即使头部一个字节都没收到就遇到 EOF，也会抛异常；不能用空字符串代表 EOF，因为空正文合法。

## 6. 应用层状态与失败策略

| 状态 | 含义 | 描述符 |
| --- | --- | --- |
| `CREATED` | socket 创建成功 | 打开 |
| `BOUND` | 绑定成功 | 打开 |
| `LISTENING` | 监听成功 | 打开 |
| `CONNECTED` | 连接成功或接管已接受连接 | 打开 |
| `PEER_CLOSED` | 观察到对端发送结束，随后关闭本端 | 已释放，成员值为 `-1` |
| `TIMED_OUT` | 接收超时并关闭 | 已释放，成员值为 `-1` |
| `FAILED` | 致命收发、协议接收失败或 `Abort()` | 已释放，成员值为 `-1` |
| `CLOSED` | 主动 `Close()` | 已释放，成员值为 `-1` |

`RequireState()` 约束：绑定和客户端连接要求 `CREATED`，监听要求 `BOUND`，接受连接要求 `LISTENING`，收发要求 `CONNECTED`。不符合要求则抛出 `std::logic_error`。这些是封装约定，并不是 Linux 支持的全部操作顺序。

建立阶段成功后才更新状态。绑定、监听或连接失败时，当前实现保留原应用状态并抛异常，但原状态不表示内核状态已恢复；尤其不能据此直接重试失败的 `connect()`，应结束对象并重新创建 socket。

协议接收使用 `catch (...)` 清理资源，再通过 `throw;` 原样抛出异常。如果底层已关闭且记录 `TIMED_OUT` 或 `PEER_CLOSED`，后续 `Abort()` 不覆盖结束原因。

TCP 半关闭不强制本端也关闭，这里发现 EOF 就结束整条连接是项目策略。半帧失败前已读走的字节不会自动退回内核缓冲区；当前没有保存可恢复的解析进度，因此不复用失败连接。

`CONNECTED` 只表示本地尚未观察到终止，不是实时健康检测。这些应用状态不是内核的 `ESTABLISHED`、`CLOSE_WAIT`、`TIME_WAIT`。

## 7. 测试覆盖

| CTest 项目 | 验证内容 |
| --- | --- |
| `tcp_socket_test` | 描述符有效且不同、析构释放、端口分配、监听标志、非法参数、连接两端端口、独立连接关闭、请求响应、嵌入零字节、提前 EOF |
| `message_protocol_test` | 普通/空/二进制/最大长度正文、连续帧、本地超长不污染流、超长消息头、头和正文截断 |
| `socket_timeout_test` | 非法超时参数、正常通信、无数据/半头/半正文停滞 |
| `connection_lifecycle_test` | 状态迁移、主动和异常关闭、实际释放描述符、关闭后禁止收发、重复关闭保留原因、监听对象不受影响、本地超长不误关连接 |

测试使用 `Require()` 抛异常，不依赖可能被 Release 构建禁用的 `assert`。四个 CTest 项目各有多个检查，不等于只有四条断言；测试函数必须在入口中实际调用。

关闭测试使用保存的描述符调用 `fcntl(F_GETFD)`，检查返回 `-1` 且 `errno == EBADF`，检查前不创建新描述符以免号码复用。异常测试只接受预期错误，其他异常重新抛出使测试失败。

现有测试未强制注入所有短写、`EINTR`、TCP RST 和内存分配失败，不能宣称所有故障路径均已验证。

## 8. 验证记录与异常日志

2026-09-13，在 WSL Ubuntu-24.04 重新构建并执行四个 CTest 项目、演示和生命周期测试，均正常结束。此次 CTest 总耗时约 1.59 秒，耗时随环境变化。

```text
1/4 Test #1: tcp_socket_test .................. Passed
2/4 Test #2: message_protocol_test ............ Passed
3/4 Test #3: socket_timeout_test .............. Passed
4/4 Test #4: connection_lifecycle_test ........ Passed
100% tests passed, 0 tests failed out of 4
```

演示输出节选：

```text
[server] state=CONNECTED
[server] 收到请求: GET_PORT Ethernet0
[client] 收到响应: Ethernet0 UP 10000Mbps
[server] state=CLOSED
[client] state=CLOSED
```

实际异常日志节选：

```text
[normal] CONNECTED -> CLOSED
[truncated-body] CONNECTED -> PEER_CLOSED | fd=5 | reason: peer ended sending before all expected bytes arrived
[idle-timeout] CONNECTED -> TIMED_OUT | fd=5 | reason: receive timeout: Resource temporarily unavailable
[oversized-header] CONNECTED -> FAILED | fd=5 | reason: message too long
All connection lifecycle tests passed
```

日志由测试程序输出，并非独立日志模块。`fd=5` 是关闭前保存的号码，不同场景依次运行时可以重复使用；端口、描述符和系统错误文字可能随环境变化。

需要保存完整日志时，可执行以下命令（会覆盖同名日志）：

```bash
./build/connection_lifecycle_test > build/connection_lifecycle.log 2>&1
```

也可以用 `ctest --test-dir build -V` 查看测试详细输出。

## 9. 已知限制与后续方向

- 仅支持 Linux IPv4 回环通信，没有远程地址配置、DNS、IPv6 或 TLS。
- 演示是单线程、小消息顺序收发，不是多连接事件循环或真实端口查询服务。
- 只有接收调用超时，没有发送超时、连接建立超时、整帧总期限或慢速发送防护。
- 没有半帧恢复、自动重连、完整半关闭会话或实时健康探测。
- 对象没有线程同步，不支持多线程无协调地操作同一对象；长度头和正文的两次发送也不保证跨线程原子性。
- 没有显式移动所有权支持。`GetFd()` 用于观察与测试，不应由外部随意关闭或修改阻塞模式，以免破坏内部约定。
- 没有独立日志系统、负载测试、背压处理、epoll 或定时器驱动。

后续基于本周的所有权、报文边界和错误处理经验，学习非阻塞 socket、每连接缓冲区、事件循环和总截止时间。迁移时需要重新处理 `EAGAIN`，不能直接把阻塞接收循环搬进 epoll 回调。
