#pragma once

#include "epoll_poller.h"
#include "io_utils.h"
#include "timer_fd.h"
#include "logger.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <string_view>

namespace net {
    class TcpServer {
        public:
            // port 为 0：由操作系统分配可用端口。
            // 第二个参数控制统计周期，默认每 1000 毫秒一次。
            // 保留默认值，所以原来的 TcpServer server; 仍然可以使用。
            // 新增 idleTimeoutMs：空闲超时时间，0 表示禁用清理。
            explicit TcpServer(std::uint16_t port = 0, int statisticsIntervalMs = 1000, int idleTimeoutMs = 5000,
            // Logger* 表示可选的、不归 TcpServer 所有的日志对象。
            // nullptr 表示本次运行不输出服务端内部日志。
            Logger* logger = nullptr
            );

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

            // 空闲超时关闭数，是总关闭数的一部分。
            std::size_t IdleClosedCount() const noexcept {
                return idleClosedCount_;
            }

        private:
            void AcceptReady();
            void HandleClient(int fd, std::uint32_t events);
            void CloseClient(int fd);
            void HandleTimer();

            EpollPoller poller_;
            // 拥有指针：TcpServer 只使用 Logger，不负责销毁它。
            // 外部 Logger 的生命周期必须长于 TcpServer。
            Logger* logger_{nullptr};
            UniqueFd listener_;
            TimerFd statisticsTimer_;
            
            // 时钟类型别名，用于计算经过的时长。
            using Clock = std::chrono::steady_clock;

            // 每条连接同时保存资源和最后活动时间。
            struct Connection {
                UniqueFd socket;    
                Clock::time_point lastActive;
            };
            void CloseIdleConnections();

            int idleTimeoutsMs_{0};
            std::size_t idleClosedCount_{0};
            
            // 集中处理日志输出。
            // TcpServer 不直接依赖 cout 或 cerr。
            void WriteLog(LogLevel level, std::string_view message);

            // key：连接的 fd。
            // value：真正拥有这个 fd 的 RAII 对象。
            //
            // 删除一项时，UniqueFd 析构，连接随之关闭。
            std::unordered_map<int, Connection> connections_;
            std::uint16_t port_{0};

            // 本练习所有服务端操作都发生在同一线程，不需要 atomic。
            std::size_t acceptedCount_{0};
            std::size_t closedCount_{0};
            std::size_t receivedBytes_{0};
            std::uint64_t timerTicks_{0};
    };
} // namespace net
