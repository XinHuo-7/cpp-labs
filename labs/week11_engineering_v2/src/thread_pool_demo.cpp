#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount) {
        if (threadCount == 0) {
            throw std::invalid_argument("threadCount must be positive");
        }
        try {
            for (std::size_t i = 0; i < threadCount; ++i) {
                // 每次 emplace_back 都创建一个 std::thread。
                // [this] 让线程调用当前 ThreadPool 对象的 WorkerLoop。
                workers_.emplace_back([this] {WorkerLoop(); });
            }
        } catch (...) {
            // 如果创建到一半失败，也要回收已经启动的线程。
            Stop();
            throw;
        }
    }

    ~ThreadPool() {
        Stop();
    }
    // 线程池拥有线程和互斥量，不能简单复制。
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool operator=(const ThreadPool&) = delete;

    bool Sumbmit(std::function<void()> task) {
        if (!task) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                return false;
            }

            // 把任务移入队列；队列和 stopping_ 都由同一把锁保护。
            tasks_.push(std::move(task));
        }

        ready_.notify_one(); // 唤醒一个等待任务的工作线程。
        return true;
    }

    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true; // 从此不再接受新任务。
        }
        ready_.notify_all(); // 唤醒所有线程，让它们有机会退出。

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join(); // 等待线程处理完队列中的任务。
            }
        }

    }

    std::size_t FailedTaskCount() const noexcept {
        return failedTasks_.load(std::memory_order_relaxed);
    }
private:
    void WorkerLoop() {
        for (;;) {
            std::function<void()> task;
            {
                // condition_variable::wait 需要 unique_lock：
                // 等待期间它会暂时解锁，醒来后重新加锁。
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [this] {
                    return stopping_ || !tasks_.empty();
                });
                // 停止后仍要先做完队列中的任务；队列空了才退出。
                if (stopping_ && tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            } // 此处释放锁；执行任务时不占用队列的锁。
            try {
                task();
            } catch(...) {
                // Day 1 只记录失败数量，不让任务异常终止整个进程。
                failedTasks_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable ready_;
    bool stopping_{false};
    std::atomic<std::size_t> failedTasks_{0};
};

int main() {
    ThreadPool pool{2};
    std::atomic<int> completed{0};

    for (int i = 0; i < 8; ++i) {
        const bool accepted = pool.Sumbmit([&completed] {
            completed.fetch_add(1, std::memory_order_relaxed);
        });

        if (!accepted) {
            throw std::runtime_error("task was rejected unexpectedly");
        }
    }

    pool.Stop();

    // Stop 返回时，两个工作线程都已结束，已有任务应全部完成。
    assert(completed.load(std::memory_order_relaxed) == 8);
    assert(pool.FailedTaskCount() == 0);
    
    const bool acceptedAfterStop = pool.Sumbmit([] {});
    assert(!acceptedAfterStop);
    std::cout << "THreadPool test passed: completed=8\n";
}

