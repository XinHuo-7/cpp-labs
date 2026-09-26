#include "epoll_poller.h"

#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace net {
    EpollPoller::EpollPoller() :epollFd_(::epoll_create1(EPOLL_CLOEXEC)) {
        // EPOLL_CLOEXEC：设置 close-on-exec，
        // 成功执行 exec 替换程序时，自动关闭这个描述符。
        // 它不是非阻塞标志，也不会创建线程。

        if (!epollFd_.IsValid()) {
            const int erroCode = errno;
            throw std::system_error(
                erroCode,
                std::generic_category(),
                "epoll_create! failed"
            );
        }

    }

    int EpollPoller::GetFd() const noexcept {
        return epollFd_.Get();
    }

    void EpollPoller::Add(int fd, std::uint32_t events) {
        epoll_event event{};

        // events 是位掩码，表示关注哪些事件。
        event.events = events;

        // data 是用户附带的数据。
        // 这里保存 fd，便于 Wait 返回时识别是哪一个 socket。
        event.data.fd = fd;

        const int result = ::epoll_ctl(
            epollFd_.Get(), // 操作哪个epoll 实例
            EPOLL_CTL_ADD,   // 添加关注对象
            fd,              // 被关注的描述符
            &event           // 关注的事件及附带数据
        );

        if (result == -1) {
            const int errorCode = errno;

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "epoll ADD failed"
            );
        }
    }

    void EpollPoller::Remove(int fd) {
        const int result = ::epoll_ctl(
            epollFd_.Get(),
            EPOLL_CTL_DEL, // 仅删除关注关系，不关闭 fd
            fd,
            nullptr        // 删除操作不需要提供新的事件配置
        );
        if (result == -1) {
            const int errorCode = errno;

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "epoll DEL failed"
            );
        }
    }

    std::vector<epoll_event> EpollPoller::Wait(int timeoutMs) {
        if (timeoutMs < -1) {
            throw std::invalid_argument("timeout must be -1 or nonnegative");
        }

        // 每次最多取 16 个事件，不代表最多只能注册 16 个 fd。
        constexpr int kMaxEvents = 16;

        // 必须创建实际元素作为可写空间，不能只 reserve 容量。
        std::vector<epoll_event> events(kMaxEvents);

        const int count = ::epoll_wait(
            epollFd_.Get(),
            events.data(), // 内核把事件写入这块连续空间
            kMaxEvents,
            timeoutMs
        );

        if (count == -1) {
            const int errorCode = errno;
            // EINTR 表示等待过程被信号中断。
            // 它不代表 epoll 或服务端发生故障。
            //
            // 返回空事件集合，让 main 循环有机会检查
            // IsShutdownRequested()。
            if (errorCode == EINTR) {
                events.clear();
                return events;
            }

            // Day2 暂时统一报告错误，包括 EINTR。
            // 不盲目用完整 timeout 重试，以免反复延长总等待时间。
            throw std::system_error(
                errorCode,
                std::generic_category(),
                "epoll_wait failed"
            );    
        }

        // 内核只写入 count 个有效事件，去掉后面的占位元素。
        // count == 0 时得到空 vector。
        events.resize(static_cast<std::size_t>(count));
        return events;
    }
}