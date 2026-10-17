#include "thread_pool.h"

int main() {
    // 1 个工作线程、1 个等待位置，方便准确验证“队列已满”。
    ThreadPool pool{1, 1};
    std::atomic<int> completed{0};

    std::promise<void> startedSignal;
    auto started = startedSignal.get_future();

    std::promise<void> releaseSignal;
    auto released = releaseSignal.get_future().share();

    const bool firstAccepted = pool.Sumbmit([&] {
        // 通知主线程：第一个任务已被工作线程取走。
        startedSignal.set_value();

        // 暂时不结束，让唯一的工作线程保持忙碌。
        released.wait();
        completed.fetch_add(1, std::memory_order_relaxed);
    });
    assert(firstAccepted);
    started.wait();

    // 第一个任务正在执行，因此队列目前是空的。
    const bool secondAccepted = pool.Sumbmit([&] {
        completed.fetch_add(1, std::memory_order_relaxed);
    });

    // 第二个任务占满唯一的排队位置，第三个应被拒绝。
    const bool thirdAccepted = pool.Sumbmit([&] {
        completed.fetch_add(1, std::memory_order_relaxed);
    });

    assert(secondAccepted);
    assert(!thirdAccepted);

    releaseSignal.set_value();   // 允许第一个任务结束。
    pool.Stop();                 // 等待前两个任务完成。

    assert(completed.load(std::memory_order_relaxed) == 2);
    assert(pool.FailedTaskCount() == 0);

    std::cout << "Bounded queue test passed: completed=2, rejected=1\n";
}

