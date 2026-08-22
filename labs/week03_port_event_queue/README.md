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

##调试记录

问题：vector 扩容后访问缓存 Port*，出现悬空指针。

复现条件：
1. reserve(3) 后填满 vector；
2. 缓存第一个元素地址；
3. 再 emplace_back 一个新端口触发扩容；
4. 访问旧指针。

证据：
- GDB：capacity 从 3 变为 6；
- ASan：heap-use-after-free，定位到旧指针访问行。

根因：
vector 重分配了底层连续内存，旧元素地址失效。

修复：
扩容后不再访问旧指针；通过索引、名称重新查找，或重新获得有效引用。