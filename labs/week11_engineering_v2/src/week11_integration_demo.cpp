#include "event_loop.h"
#include "thread_pool.h"
#include "timer_queue.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

int main() {
    using namespace std::chrono_literals;

    // 创建顺序也确定了销毁顺序：销毁时先 TimerQueue，
    // 再 ThreadPool，最后 EventLoop
    EventLoop loop;
    ThreadPool pool{2, 2};
    TimerQueue timers;

    const auto mainThreadId = std::this_thread::get_id();
    std::thread::id computationThreadId;
    std::thread::id resultThreadId;
    int result = -1;
    
    const bool scheduled = timers.ScheduleAfter(20ms, [&] {
        // 这段首先在 TimerQueue 的定时器线程中运行。
        const bool accepted = pool.Sumbmit([&] {
            // 这段由 ThreadPool 的某个工作线程运行。
            const int computed = 6 * 7; // 工作线程计算
            const auto wokerId = std::this_thread::get_id();

            // computed、workerId 按值捕获：工作任务结束后，
            // EventLoop 仍能安全使用这两个值。
            loop.Post([&, computed, wokerId] {
                // 这段由运行 loop.Run() 的主线程执行。
                result = computed;      // 主线程处理结果
                computationThreadId = wokerId;
                resultThreadId = std::this_thread::get_id();
                loop.Stop();
            });
        });
        if (!accepted) {
            // 提交失败也要让 Run() 退出，由下面的检查报告失败。
            loop.Post([&] {loop.Stop();});
        }
    });

    // 测试保护：正常流程应很快结束；否则最多约 1 秒后叫醒 Run()。
    const bool timeoutScheduled = timers.ScheduleAfter(1000ms, [&] {
        loop.Stop();
    });
    if (!scheduled || !timeoutScheduled) {
        throw std::runtime_error("failed to schedule timer");
    }

    loop.Run(); // 主线程在这里等待并处理 Post 进来的结果。
    // 先停止产生新工作的定时器，再等待线程池已有任务结束。
    timers.Stop();
    pool.Stop();

    assert(result == 42);
    assert(computationThreadId != mainThreadId);
    assert(resultThreadId == mainThreadId);
    assert(timers.FailedCallbackCount() == 0);
    assert(pool.FailedTaskCount() == 0);
    assert(loop.FailedTaskCount() == 0);

    std::cout << "week 11 integration passed: result = 42 \n";
   
}