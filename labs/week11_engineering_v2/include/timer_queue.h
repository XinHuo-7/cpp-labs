#pragma once
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

class TimerQueue {
public:
    TimerQueue() : worker_([this] {Run();}) {}

    ~TimerQueue() { Stop(); }

    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator =(const TimerQueue&) = delete;

    bool ScheduleAfter(std::chrono::milliseconds delay, std::function<void()> callback) {
        if (delay.count() < 0 || !callback) {
            return false;
        }

        // steady_clock 不受系统时钟被手动调整的影响，适合计算“多久后到期”。
        const auto deadline = Clock::now() + delay;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                return false;
            }
            // multimap 按到期时间排序；begin() 始终是最早到期的任务。
            timers_.emplace(deadline, std::move(callback));
        }

        // 新任务可能比原来的任务更早到期，需要唤醒工作线程重新计算。
        ready_.notify_one();
        return true;
    }

    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
            timers_.clear(); // 本练习约定：停止时取消尚未开始的定时任务。
        }
        ready_.notify_all();
        if (worker_.joinable()) {
            worker_.join(); // 如果回调正在执行，等待它执行完。
        }
    }

    std::size_t FailedCallbackCount() const noexcept {
        return failedCallbacks_.load(std::memory_order_relaxed);
    }

private:
    using Clock = std::chrono::steady_clock;
    void Run() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!stopping_) {
            if (timers_.empty()) {
                // 没有任务时等待；等待期间会释放 mutex_。
                ready_.wait(lock, [this] {
                    return stopping_ || !timers_.empty();
                });
                continue;
            }

            const auto deadline = timers_.begin()->first;

            // 最多等到最近的到期时间。
            // 若有人加入了更早到期的任务，就提前返回并重新计算。
            const bool needRecheck = ready_.wait_until(
                lock,
                deadline,
                [this, deadline] {
                    return stopping_ || (!timers_.empty() && timers_.begin()->first < deadline);
                }
            );

            if (needRecheck) {
                continue;
            }

            // 等到 deadline 后，取出最早的回调。
            auto callback = std::move(timers_.begin() -> second);
            timers_.erase(timers_.begin());

            lock.unlock(); // 执行回调时不占用队列的锁。
            try {
                callback();
            } catch(...) {
                failedCallbacks_.fetch_add(1, std::memory_order_relaxed);
            }
            lock.lock();
        }
        
    }
    std::mutex mutex_;
    std::condition_variable ready_;
    std::multimap<Clock::time_point, std::function<void()>> timers_;
    bool stopping_{false};
    std::atomic<std::size_t> failedCallbacks_{0};

    // 放在成员列表最后：线程启动时，上面的成员都已经构造完成。
    std::thread worker_;
};