#pragma once

#include "epoll_poller.h"
#include "io_utils.h"
#include "timer_fd.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace net {
    class TcpServer {
        public:
            // port 为 0：由操作系统分配可用端口。
            // 第二个参数控制统计周期，默认每 1000 毫秒一次。
            // 保留默认值，所以原来的 TcpServer server; 仍然可以使用。
            explicit TcpServer(std::uint16_t port = 0, int statisticsIntervalMs = 1000);

            // 成员对象负责清理资源：
            // connections_ 关闭客户端连接，listener_ 关闭监听 socket。
            ~TcpServer() = default;

            TcpServer(const TcpServer&) = delete;
            TcpServer& operator = (const TcpServer&) = delete;

            // 执行一轮事件处理，不是在函数内部永久循环。
            void RunOnce(int timeoutMs);

            std::uint16_t GetPort() const noexcept {
                return port_;
            }

            std::size_t ConnectionCount() const noexcept {
                return connections_.size();
            }

            std::size_t AcceptedCount() const noexcept {
                return acceptedCount_;
            }

            std::size_t ClosedCount() const noexcept {
                return closedCount_;
            }

            std::size_t ReceivedBytes() const noexcept {
                return receivedBytes_;
            }
            // 返回已经消费的累计到期次数，主要用于观察和测试。
            // 仍是单线程访问，不需要 atomic。
            std::uint64_t TimerTicks() const noexcept {
                return timerTicks_;
            }

        private:
            void AcceptReady();
            void HandleClient(int fd, std::uint32_t events);
            void CloseClient(int fd);
            void HandleTimer();

            EpollPoller poller_;
            UniqueFd listener_;
            TimerFd statisticsTimer_;

            // key：连接的 fd。
            // value：真正拥有这个 fd 的 RAII 对象。
            //
            // 删除一项时，UniqueFd 析构，连接随之关闭。
            std::unordered_map<int, UniqueFd> connections_;
            std::uint16_t port_{0};

            // 本练习所有服务端操作都发生在同一线程，不需要 atomic。
            std::size_t acceptedCount_{0};
            std::size_t closedCount_{0};
            std::size_t receivedBytes_{0};
            std::uint64_t timerTicks_{0};
    };
} // namespace net
