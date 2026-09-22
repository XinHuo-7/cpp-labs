#include "tcp_server.h"

#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace net {
    TcpServer::TcpServer(std::uint16_t port, int statisticsIntervalMs, int idleTimeoutMs) : listener_(::socket(
        AF_INET,
        SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
        0
    )) {
        // SOCK_NONBLOCK：监听 socket 的 accept 操作不能一直阻塞。
        // SOCK_CLOEXEC：成功 exec 后自动关闭该描述符。
        if (!listener_.IsValid()) {
            throw std::system_error(errno, std::generic_category(), "socket failed");
        }

        if (idleTimeoutMs < 0) {
            throw std::invalid_argument("idle timeout must be nonnegative");
        }
        idleTimeoutsMs_ = idleTimeoutMs;


        sockaddr_in address{};
        address.sin_family  = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(port);

        if (::bind(listener_.Get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1) {
            throw std::system_error(errno, std::generic_category(), "bind failed");
        }

        // 16 是等待 accept 的连接队列参数
        if (::listen(listener_.Get(), 16) == -1) {
            throw std::system_error(errno, std::generic_category(), "listen failed");
        }

        // 当传入的 port 为 0 时，通过 getsockname 查询实际端口。
        socklen_t length = sizeof(address);
        if (::getsockname(listener_.Get(), reinterpret_cast<sockaddr*>(&address), &length) == -1) {
            throw std::system_error (errno, std::generic_category(), "getsockname failed");
        }

        port_ = ntohs(address.sin_port);

        // 监听 socket 可读，表示可以尝试接受新连接。
        // 没有 EPOLLET，因此采用 LT
        poller_.Add(listener_.Get(), EPOLLIN);

        // 将定时器与 socket 注册到同一个 epoll 实例。
        poller_.Add(statisticsTimer_.GetFd(), EPOLLIN);

        // 第一次和后续间隔相同：形成周期定时器。
        // 例如 1000, 1000 表示约 1 秒后首次到期，之后每秒到期。
        statisticsTimer_.Start(statisticsIntervalMs, statisticsIntervalMs);
    }

void TcpServer::AcceptReady() {
        // 每轮限制 accept 尝试次数，避免连接不断涌入时，
        // 一直留在这里，迟迟不处理已有连接。
        //
        // 使用 LT：如果仍有连接待接受，后续还会收到通知。
        constexpr int kAcceptBudget = 32;

        for (int attempt = 0; attempt < kAcceptBudget; ++attempt) {
            // 不需要获取客户端地址，因此地址参数传 nullptr。
            //
            // 必须显式设置新连接的非阻塞标志，
            // 不能依赖它继承监听 socket 的 O_NONBLOCK。
            const int fd = ::accept4(
                listener_.Get(),
                nullptr,
                nullptr,
                SOCK_NONBLOCK | SOCK_CLOEXEC);

            if (fd == -1) {
                const int error = errno;

                if (error == EINTR || error == ECONNABORTED) {
                    continue;
                }

                if (error == EAGAIN || error == EWOULDBLOCK) {
                // 当前待接受的连接已经取完，不是服务端故障。
                    return;
                }
                // 本练习其他 accept 错误交给外层处理。
                throw std::system_error(
                error, std::generic_category(), "accept4 failed");
            }

            // 先交给 RAII 管理，后续操作抛异常时也不会泄漏 fd。
            UniqueFd connection(fd);

            // std::move 将 fd 的所有权转移到容器中的 UniqueFd。
            // 不是复制 fd 的所有权，也没有创建新的 socket。
            // 接受连接时开始计算空闲时间。
            // 两个初始化值分别对应 socket 和 lastActive。
            connections_.emplace(fd, Connection{std::move(connection), Clock::now()});

            try {
                // EPOLLRDHUP：关注对端关闭发送方向。
                // 最终仍通过读取结果确认是否已经读到 EOF。
                poller_.Add(fd, EPOLLIN | EPOLLRDHUP);
            } catch (...) {
                // 注册失败时回滚：
                // 删除容器项，触发 UniqueFd 析构并关闭 fd。
                connections_.erase(fd);
                throw;
            }
            ++acceptedCount_;

            std::cout << "[accept] fd = " << fd << '\n';          

        }
    }

void TcpServer::CloseClient(int fd) {
    const auto it = connections_.find(fd);

    if (it == connections_.end()) {
        return;
    }

    // Remove 只取消 epoll 关注关系，不关闭 socket。
    poller_.Remove(fd);

    // erase 销毁保存的 UniqueFd，真正关闭 socket。
    connections_.erase(it);

    ++closedCount_;
    std::cout << "[close] fd=" << fd << '\n';
}

void TcpServer::HandleClient(int fd, std::uint32_t events) {
    const auto it = connections_.find(fd);
    // 保存迭代器，后面通过它更新连接的活动时间。
    if (it == connections_.end()) {
        return;
    }

    // 即使同时出现关闭通知，也先尝试读取，
    // 避免直接丢掉对端关闭前已经发送的数据。
    if ((events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR)) == 0) {
        return;
    }

    bool shouldClose = false;

    try {
        const auto result = DrainReadable(fd);
        if (!result.data.empty()) {
            // 新增：只有实际读到数据才刷新时间。
            it->second.lastActive = Clock::now();
            receivedBytes_ += result.data.size();
            std::cout << "[recv] fd = " << fd
                      << "bytes= " << result.data.size() << '\n';
            // 当前只统计收到的字节，不把这一批数据当作完整消息。
            // 后续接入协议层时，需要维护每条连接的接收缓冲区。
        }

        // data 非空与 peerClosed 为 true 可以同时成立：
        // 必须先处理 data，再关闭连接。
        shouldClose = result.peerClosed;
    } catch (const std::system_error& error) {
        // 单条连接的接收错误只结束这条连接。
        std::cerr << "[recv error] fd = " << fd << " reason=" << error.what() << '\n';
        shouldClose = true;
    } catch (const std::length_error& error) {
        // 沿用 Day3 策略：单次接收批次超限，放弃该连接。
        std::cerr << "[receive limit] fd=" << fd
                  << " reason=" << error.what() << '\n';
        shouldClose = true;
    }

    // 本练习采用简单策略：发生错误或完全挂断时关闭连接。
    // 注意 EPOLLRDHUP 不放在这里：它仍需要通过读到 EOF 处理。
    if ((events & (EPOLLERR | EPOLLHUP)) != 0) {
        shouldClose = true;
    }

    if (shouldClose) {
        CloseClient(fd);
    }
}

void TcpServer::RunOnce(int timeoutMs) {
    const auto events = poller_.Wait(timeoutMs);

    bool listenerReady = false;
    bool timerReady = false;  // 新增：记录本批是否有定时事件。

    for (const auto& event : events) {
        const int fd = event.data.fd;

        // 新增分支：这是定时器事件，不是网络数据。
        if (fd == statisticsTimer_.GetFd()) {
            if ((event.events & (EPOLLERR | EPOLLHUP)) != 0) {
                throw std::runtime_error("statistics timer failed");
            }
            if ((event.events & EPOLLIN) != 0) {
                // 先记录，让本批客户端数据先得到处理并刷新活动时间。
                timerReady = true;
            }
            
            // 已处理完，不再交给下面的 socket 分支。
            continue;
        }
        if (fd == listener_.Get()) {
            if ((event.events & (EPOLLERR | EPOLLHUP)) != 0) {
                throw std::runtime_error("listener failed");
            }

            if ((event.events & EPOLLIN) != 0) {
                listenerReady = true;
            }
            continue;
        }

        HandleClient(fd, event.events);
    }
    // 本批已有连接处理完后，再检查超时。
    if (timerReady) {
        HandleTimer();
    }
    // 先处理这一批已有连接，再接受新连接。
    // 避免旧连接关闭后 fd 被新连接复用，
    // 使本批剩余事件被误认为属于新连接。
    if(listenerReady) {
        AcceptReady();
    }
}

void TcpServer::HandleTimer() {
    // 必须读取到期次数。
    // 在 LT 模式下，不读取会让 fd 一直保持可读，
    // 导致后续 epoll_wait 反复立即返回。

    const auto expirations = statisticsTimer_.Consume();

    if (expirations == 0) {return;}
    timerTicks_ += expirations;
    CloseIdleConnections();

    // 即使积累了多个到期次数，也只输出一份当前统计。
    // 到期计数与实际执行统计输出的次数不是同一个概念。
    std::cout << "[stats]"
              << " ticks=" << timerTicks_
              << " active=" << connections_.size()
              << " accepted=" << acceptedCount_
              << " closed=" << closedCount_
              << " bytes=" << receivedBytes_
              << " idleClosed=" << idleClosedCount_
              << '\n';
}

void TcpServer::CloseIdleConnections() {
    if (idleTimeoutsMs_ == 0) {
        return;
    }
    const auto now = Clock::now();
    const auto timeout = std::chrono::milliseconds(idleTimeoutsMs_);
    std::vector<int> expireFds;
    for (const auto& item : connections_) {
        // item.first 是 fd，item.second 是 Connection。
        const auto idleDuration = now - item.second.lastActive;
        if (idleDuration >= timeout) {
            expireFds.push_back(item.first);
        }
    }
    // 先收集，再删除，避免遍历过程中删除当前元素使迭代器失效。
    for (const int fd : expireFds) {
        std::cout << "[idle timeout] fd=" << fd << '\n';
        CloseClient(fd);
        ++idleClosedCount_;
    }
}

} // namespace net