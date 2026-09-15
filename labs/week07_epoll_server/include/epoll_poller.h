#pragma once

#include "io_utils.h"

#include <cstdint>
#include <vector>

#include <sys/epoll.h>

namespace net {
    class EpollPoller {
        public:
            EpollPoller();

             // 默认析构会析构成员 epollFd_，由 UniqueFd 关闭 epoll 描述符。
             ~EpollPoller() = default;

             // epoll 实例采用独占所有权，不允许复制
             EpollPoller(const EpollPoller&) = delete;
             EpollPoller& operator=(const EpollPoller&) = delete;

             int GetFd() const noexcept;

            // 默认关注可读事件，不设置 EPOLLET，因此是 LT 模式。
            // 这里只注册关注关系，不接管 fd 的所有权。
            void Add(int fd, std::uint32_t events = EPOLLIN);

            // 取消关注，但不会关闭被关注的fd
            void Remove(int fd);

            // timeoutMs：
            //   0：立即检查，不等待
            //  >0：最多等待指定毫秒数（实际返回受调度影响）
            //  -1：无限等待

            // 返回空 vector 表示本次没有取得事件，不表示连接关闭。
            std::vector<epoll_event> Wait(int timeoutMs);
        
        private:
            UniqueFd epollFd_;

    };
}