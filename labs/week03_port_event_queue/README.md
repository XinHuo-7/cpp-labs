# Week 03 - Port Event Queue

## 功能
- FIFO 端口事件队列
- 端口注册与状态更新
- 重复事件、未知端口处理
- CTest 单元测试

## 架构
- PortEvent：端口名与目标链路状态
- EventQueue：deque 实现 FIFO
- PortStateTable：unordered_map 保存端口当前状态
- CLI：提交、消费并显示处理结果

## 构建与测试
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/port_event_queue_cli
ctest --test-dir build --output-on-failure

## 本周知识
- vector/deque/unordered_map 的选择
- 指针与迭代器失效
- 多文件 CMake
- GDB 调试
- AddressSanitizer