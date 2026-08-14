# Week 01 - Port Manager

一个使用 Modern C++ 和 CMake 实现的简单端口管理 CLI 项目。

## 功能

- 添加端口及速率
- 查询指定端口
- 将端口状态设置为 UP / DOWN
- 列出全部端口
- 拒绝重复端口名和非法速率
- 使用 CTest 自动验证核心逻辑

## 项目结构

```text
week01_port_manager/
├── include/
│   ├── port.h                 # Port 类声明
│   └── port_manager.h         # PortManager 类声明
├── src/
│   ├── main.cpp               # CLI 入口和命令解析
│   ├── port.cpp               # Port 类实现
│   └── port_manager.cpp       # PortManager 类实现
├── tests/
│   └── port_manager_test.cpp  # 自动化测试
├── CMakeLists.txt
└── README.md
```

## 技术点

- C++20
- 类与封装：`Port`、`PortManager`
- `enum class`：端口链路状态
- `std::unordered_map`：使用端口名快速查找端口
- 引用、指针、`const`
- `try_emplace`、`find`
- CMake：核心库、CLI、测试程序分层构建
- CTest：自动化测试

## 构建与运行

在项目根目录执行：

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/port_manager_cli
```

## CLI 命令

```text
add <name> <speedMbps>  添加端口
up <name>               设置端口为 UP
down <name>             设置端口为 DOWN
show <name>             查询指定端口
list                    列出所有端口
help                    查看帮助
quit                    退出程序
```

示例：

```text
> add Ethernet0 10000
端口添加成功。

> up Ethernet0
端口状态更新成功。

> show Ethernet0
端口: Ethernet0 | 状态: UP | 速率: 10000 Mbps

> list
端口总数: 1
端口: Ethernet0 | 状态: UP | 速率: 10000 Mbps
```

## 自动化测试

构建完成后执行：

```bash
ctest --test-dir build --output-on-failure
```

预期结果：

```text
100% tests passed, 0 tests failed out of 1
```

## 网络研发关联

该项目模拟网络设备软件中“端口状态管理”的基础逻辑：

- `Port` 对应设备端口的抽象；
- `PortManager` 对应端口状态管理模块；
- CLI 对应调试或运维入口；
- 自动化测试用于保证后续修改不破坏既有功能。

后续可继续扩展端口速率校验、状态变更事件、配置文件加载和网络通信接口。