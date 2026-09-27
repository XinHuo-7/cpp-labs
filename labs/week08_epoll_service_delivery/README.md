# Week 08：epoll 服务工程交付

本项目在 Week 07 的单线程 epoll TCP 服务端基础上，补齐日志、命令行配置、信号退出、帮助信息和安装规则。目标是让程序不仅能在构建目录里运行，也能按文档完成构建、测试和本地交付。

> 范围：这是 Linux/WSL 上的教学项目。服务端监听 `127.0.0.1`，接收并统计原始字节；目前不发送响应，也没有接入长度前缀消息协议。`install` 只是把已编译程序放到指定目录，不代表生成了跨平台或独立于系统库的安装包。

## 本周内容

| 日期 | 主题 | 本项目中的结果 |
| --- | --- | --- |
| Day 1 | 日志基础 | `Logger` 支持 DEBUG、INFO、WARN、ERROR 四级过滤，并可写入指定输出流。 |
| Day 2 | 启动配置 | `ServerConfig` 解析端口、统计周期、空闲超时和日志级别。 |
| Day 3 | 服务端接入日志 | `TcpServer` 通过外部传入的 `Logger*` 记录连接、接收、关闭和统计信息。 |
| Day 4 | 正常退出 | 收到 SIGINT 或 SIGTERM 后，主循环退出，由 RAII 关闭连接及其他文件描述符。 |
| Day 5 | 程序入口 | `--help` / `-h` 和 `--version` 不启动服务端即可输出信息。 |
| Day 6 | 本地交付 | CMake 安装最终可执行程序，并用本文记录构建和使用方法。 |
| Day 7 | 最终验收 | 待完成；不在本文预先宣称通过。 |

## 环境与目录

- Linux（本练习使用 WSL Ubuntu）；`epoll`、`timerfd` 和 `accept4` 是 Linux 接口。
- 支持 C++20 的编译器和 CMake 3.20 或更新版本。
- 在 `labs/week08_epoll_service_delivery` 目录运行以下命令。

主要文件：

```text
include/
  io_utils.h           文件描述符与非阻塞读取
  epoll_poller.h        epoll 注册和等待
  timer_fd.h           周期定时器
  tcp_server.h         服务端及连接状态
  logger.h             日志级别和输出
  server_config.h      命令行配置
  shutdown_signal.h    退出信号状态
src/
  tcp_server_demo.cpp  最终程序入口
  tcp_server.cpp       接受、读取、清理连接及统计
  logger.cpp           日志实现
  server_config.cpp    参数解析与帮助文本
  shutdown_signal.cpp  SIGINT/SIGTERM 处理
tests/                 模块和服务端关键测试
CMakeLists.txt          构建、测试与安装规则
```

`src/main.cpp`、`src/epoll_demo.cpp` 和 `src/core_debug_demo.cpp` 是保留的阶段性演示；最终服务端入口是 `src/tcp_server_demo.cpp`。

## 构建与测试

```bash
cd ~/code/cpp-labs/labs/week08_epoll_service_delivery
cmake -S . -B build/day6 -DCMAKE_BUILD_TYPE=Debug
cmake --build build/day6
ctest --test-dir build/day6 --output-on-failure
```

`cmake -S . -B build/day6` 根据当前源码生成构建文件；`cmake --build build/day6` 编译并链接；`ctest` 执行已注册的测试。这里使用独立的 `build/day6`，避免复用其他项目留下的 CMake 缓存。修改源码后，重新运行构建和测试命令。

开发时可直接运行构建目录里的程序：

```bash
./build/day6/tcp_server_demo --help
./build/day6/tcp_server_demo --version
./build/day6/tcp_server_demo --port 0 --stats-ms 1000 --idle-ms 5000 --log-level info
```

最后一条命令会持续运行。`--port 0` 让系统分配空闲端口，实际端口会写入启动日志；在终端按 Ctrl+C 请求退出。

## 安装到本地交付目录

先完成上面的构建，再执行：

```bash
cmake --install build/day6 --prefix "$PWD/build/day6/package"
./build/day6/package/bin/tcp_server_demo --help
./build/day6/package/bin/tcp_server_demo --version
```

`CMakeLists.txt` 中的 `install(TARGETS tcp_server_demo RUNTIME DESTINATION bin)` 规定只安装最终服务端。安装后的位置是 `build/day6/package/bin/tcp_server_demo`。这里没有安装到系统目录，也不需要 `sudo`。

`--build` 负责生成/更新程序；`--install` 按安装规则复制已构建的程序，**不会自动重新编译**。改代码后若要更新交付目录里的副本，先重新构建，再重新安装。只做本地开发调试时，不必执行安装，直接运行构建目录中的程序即可。

## 命令行参数

| 参数 | 默认值 | 含义 |
| --- | --- | --- |
| `--port <0-65535>` | `0` | 监听端口；`0` 表示由系统分配。服务端只绑定 `127.0.0.1`。 |
| `--stats-ms <整数>` | `1000` | 定时统计和空闲检查的周期，单位毫秒，必须大于 0。 |
| `--idle-ms <整数>` | `5000` | 连接空闲超过该时长后关闭；`0` 表示禁用空闲清理。 |
| `--log-level <级别>` | `info` | 最低输出级别：`debug`、`info`、`warning`/`warn` 或 `error`。 |
| `-h`、`--help` | — | 显示帮助并退出，不创建监听 socket。 |
| `--version` | — | 显示程序版本并退出，不创建监听 socket。 |

参数错误（例如端口越界或缺少参数值）会报告错误并以非零状态退出。当前版本常量在 `include/server_config.h` 中。

## 服务端运行过程

1. 入口解析参数。帮助和版本请求在创建日志器、监听 socket 之前直接返回。
2. 正常启动时创建 `Logger`，安装 SIGINT/SIGTERM 处理函数，并构造 `TcpServer`。
3. `TcpServer` 创建并注册监听 socket、周期 `timerfd`；新连接被接受后也加入 epoll。
4. 主线程反复调用 `RunOnce(200)`：有事件时提前处理，没有事件时最多等待约 200 毫秒，然后检查是否收到退出信号。
5. 网络事件负责接受连接、读取数据和识别对端关闭；定时器事件负责统计并检查空闲连接。
6. Ctrl+C 触发 SIGINT：处理函数只设置退出标记，主循环随后结束；对象析构时由 RAII 关闭文件描述符。

这是**单线程**事件循环，`epoll` 和 `timerfd` 本身不会为本项目额外创建工作线程。`RunOnce(200)` 的 200 毫秒是事件等待上限，不是统计周期，也不是空闲超时时间。

## 当前边界

- 只监听本机回环地址，不能直接供其他主机连接。
- 当前只统计收到的字节；一次 `recv` 的结果不等于一条完整业务消息。
- 不回发数据，也没有接入 Week 06 的消息协议。
- 退出时停止事件循环并关闭资源；没有实现请求排空、跨线程协作或生产级部署机制。
- `build/` 中的产物属于本地构建结果，不应当当作跨平台二进制发布。

## Day 6 验收记录

- [x] `cmake --build build/day6` 成功。
- [x] `ctest --test-dir build/day6 --output-on-failure` 全部通过（9/9）。
- [x] 安装目录中存在 `bin/tcp_server_demo`，且 `--help`、`--version` 正常输出。
- [x] 正常启动后可通过 Ctrl+C 退出。

以上项目请以本机实际运行结果勾选；Day 7 再做最终验收。
