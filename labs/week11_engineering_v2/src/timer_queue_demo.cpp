#include "timer_queue.h"

int main() {
    using namespace std::chrono_literals;
    TimerQueue timers;
    std::vector<int> executionOrder;

    std::promise<void> finishedSignal;
    auto finished = finishedSignal.get_future();

    // 先放入一个较晚到期的任务：工作线程可能已经开始等它。
    const bool lateAccepted = timers.ScheduleAfter(80ms, [&] {
        executionOrder.push_back(2);
        finishedSignal.set_value(); // 通知主线程：这个回调已执行
    });

    // 后放入一个更早到期的任务：必须唤醒线程重新计算等待时间。
    const bool earlyAccepted = timers.ScheduleAfter(20ms, [&] {
        executionOrder.push_back(1);
    });
    assert(lateAccepted && earlyAccepted);

    if (finished.wait_for(1s) != std::future_status::ready) {
        timers.Stop();
        throw std::runtime_error("timer callbacks did not finish");
    }

    timers.Stop();

    // Stop 已经 join 工作线程；现在读取 executionOrder 是安全的。
    assert((executionOrder == std::vector<int>{1, 2}));
    assert(timers.FailedCallbackCount() == 0);

    std::cout << "TimerQueue test passed: early -> late\n";
}