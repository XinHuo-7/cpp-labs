#pragma once
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
public:
    // maxPendingTasks 限制的是“正在排队”的任务数，
    // 不包括工作线程已经取走、正在执行的任务。
    ThreadPool(std::size_t threadCount, std::size_t maxPendingTasks) : maxPendingTasks_(maxPendingTasks) {
        if (threadCount == 0 || maxPendingTasks == 0) {
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

            // 新增：检查和入队必须在同一把锁内。
            // 否则多个提交者可能同时看见“还有空位”，导致队列超限。
            if (tasks_.size() >= maxPendingTasks_) {
                return false;
            }
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

    const std::size_t maxPendingTasks_;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable ready_;
    bool stopping_{false};
    std::atomic<std::size_t> failedTasks_{0};
};