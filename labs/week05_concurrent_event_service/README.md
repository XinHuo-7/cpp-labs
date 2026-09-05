# Port Event Service v1.0

## 功能

- 多生产者并发投递端口事件
- 单消费者后台处理
- mutex 保护事件队列
- condition_variable 实现阻塞等待
- atomic 维护服务状态与计数
- Stop 后拒绝新事件
- 已进入队列的事件处理完后退出
- 析构函数兜底执行 Stop 和 Join

## 线程模型

- main：创建服务并管理生命周期
- producerA/B/C：并发提交事件
- EventWorker：等待并处理事件

## 停止策略

本项目采用 graceful shutdown：

1. 停止接收新事件
2. 关闭事件队列
3. 处理队列中剩余事件
4. 工作线程退出
5. main 调用 Join 完成回收

## 构建运行

```bash
cmake -S . -B build
cmake --build build
./build/concurrent_event_service
ctest --test-dir build --output-on-failure