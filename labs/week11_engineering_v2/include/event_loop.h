#pragma once
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>
#include <atomic>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

class ScopeFd {
public:
    explicit ScopeFd(int fd) : fd_(fd) {
        if (fd_ == -1) {
            throw std::system_error(errno, std::generic_category(), "create fd failed");
        }
    }
    ~ScopeFd() {
        ::close(fd_);
    }
    ScopeFd(const ScopeFd&) = delete;
    ScopeFd& operator=(const ScopeFd&) = delete;

    int Get() const noexcept {
        return fd_;
    }
private:
    int fd_;
};

class EventLoop {
public:
    EventLoop()
        : epollFd_(::epoll_create1(EPOLL_CLOEXEC)),
        wakeFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
        owner_(std::this_thread::get_id()) {
            epoll_event event{};
            event.events = EPOLLIN;
            event.data.fd = wakeFd_.Get();
            // 把 eventfd(wakeFd_) 加进 epoll：有人写 eventfd 时，epoll_wait 会醒来。
            if (::epoll_ctl(epollFd_.Get(), EPOLL_CTL_ADD, wakeFd_.Get(), &event) == -1) {
                throw std::system_error(errno, std::generic_category(), "epoll_ctl failed");
            }
        }
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    bool Post(std::function<void()> task) {
        if(!task) {
            return false;
        }
        {
            // 任务队列由锁保护，允许其他线程调用 Post。
            // Stop 后拒绝新任务，但已进入队列的任务仍会执行。
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                return false;
            }
            tasks_.push(std::move(task));
        }
        Wake();
        return true;

    }
    void Run() {
        if (std::this_thread::get_id() != owner_) {
            throw std::logic_error("Run must run on the owner thread");
        }
        for (;;) {
            // -1 表示没有事件时一直等待；Wake() 能使它返回。
            RunOnce(-1);
            std::lock_guard<std::mutex> lock(mutex_);
            // 只有收到停止请求，且待执行队列已空，才结束循环。
            if (stopping_ && tasks_.empty()) {
                return;
            }
        }
    }
    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        // 主线程可能正停在 epoll_wait(-1)，必须叫醒它检查 stopping_。
        Wake();
    }
    std::size_t RunOnce(int timeoutMs) {
        if (std::this_thread::get_id() != owner_) {
            throw std::logic_error("RunOnce must run on the owner thread");
        }
        epoll_event event{};
        int readyCount;
        do {
            // 没有事件时最多等待 timeoutMs 毫秒。
            readyCount = ::epoll_wait(epollFd_.Get(), &event, 1, timeoutMs);
        } while (readyCount == -1 && errno == EINTR);

        if (readyCount == -1) {
            throw std::system_error(
                errno, std::generic_category(), "epoll_wait failed"
            );
        }
        if (readyCount == 0) {
            return 0; // 等待超时，没有任务。
        }
        // 先读取 eventfd，清除本次唤醒状态。
        std::uint64_t notifications = 0;
        ssize_t result;
        do {
            result = ::read(wakeFd_.Get(), &notifications, sizeof(notifications));
        } while (result == -1 && errno == EINTR);

        if (result != static_cast<ssize_t>(sizeof(notifications))) {
            throw std::system_error(
                errno, std::generic_category(), "eventfd read failed"
            );
        }
        std::queue<std::function<void()>> readyTaks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // 将当前待执行任务整体取出，尽快释放锁。
            readyTaks.swap(tasks_);
        }
        std::size_t executed = 0;
        while (!readyTaks.empty()) {
            auto task = std::move(readyTaks.front());
            readyTaks.pop();

            // 回调在 RunOnce 所在的线程执行，而非 Post 所在的线程。
            try {
                task();
            } catch (const std::exception& error) {
                // 一个任务失败，不应使整个 EventLoop 停止。
                failedTasks_.fetch_add(1, std::memory_order_relaxed);
                std::cerr << "[ERROR] task failed: " << error.what() << '\n';
            } catch (...) {
                // 非 std::exception 类型的异常也要隔离。
                failedTasks_.fetch_add(1, std::memory_order_relaxed);
                std::cerr << "[ERROR] task failed : unknown exception\n";
            }
            // executed 表示“尝试执行了几个任务”，包括抛异常的任务。
            ++executed;
        }
        return executed;
    }
    std::size_t FailedTaskCount() const noexcept {
        return failedTasks_.load(std::memory_order_relaxed);
    }
private:
    ScopeFd epollFd_;
    ScopeFd wakeFd_;
    std::mutex mutex_;
    std::queue<std::function<void()>> tasks_;
    const std::thread::id owner_;
    // 与 tasks_ 一样，由 mutex_ 保护。
    bool stopping_{false};
    // 只统计失败次数；不靠它控制其他数据的读写顺序。
    std::atomic<std::size_t> failedTasks_{0};

    void Wake() {
        // eventfd 内有一个 64 位计数器；写入 1 使它变为可读。
        // epoll 监视到“可读”，就会唤醒 RunOnce。
        const std::uint64_t one = 1;
        for (;;) {
            const ssize_t result = ::write(wakeFd_.Get(), &one, sizeof(one));
            
            if (result == static_cast<ssize_t>(sizeof(one))) {
                return;
            }
            if (result == -1 && errno == EINTR) {
                continue;
            }
            if (result == -1 && errno == EAGAIN) {
                // 计数器已满时必然仍可读，epoll 已有唤醒条件。
                return;
            }
            throw std::system_error(
                errno, std::generic_category(), "eventfd write failed"
            );
        }
    }
};