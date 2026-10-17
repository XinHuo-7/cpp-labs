#include "event_loop.h"

int main() {
    EventLoop loop; // 在主线程创建，因此主线程是 EventLoop 的 owner。
    std::vector<int> order;
    bool accepted[3]{false, false, false};
    

    std::thread producer([&] {
        // 生产者线程只提交，不执行这个回调。
        accepted[0] = loop.Post([&] {
            order.push_back(1);
        });
        accepted[1] = loop.Post([&] {
            throw std::runtime_error("intentional test failture");
        });
        // 请求停止：不丢弃刚刚已经接收的两个任务。
        accepted[2] = loop.Post([&] {
            order.push_back(3);
        });
        loop.Stop();
    });

    loop.Run();   // 主线程持续等待，直到 Stop 且任务做完。
    producer.join();

    assert(accepted[0] && accepted[1] && accepted[2]);
    assert((order == std::vector<int>{1, 3}));
    // assert(result == 42);
    // assert(callbackThreadId == mainThreadId);
    const bool acceptAfterStop = loop.Post([]{});
    assert(!acceptAfterStop);
    assert(loop.FailedTaskCount() == 1);

    // std::cout << "EventLoop test passed: callback ran on main thread\n";
    std::cout << "EventLoop stop test passed: 1, 2\n";
}