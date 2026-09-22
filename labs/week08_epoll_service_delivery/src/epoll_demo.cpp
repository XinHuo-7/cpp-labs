#include "epoll_poller.h"
#include "io_utils.h"

#include <cerrno>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <system_error>

#include <sys/socket.h>

int main() {
    try {
        int fds[2]{-1, -1};
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
            const int errorCode = errno;
            throw std::system_error(
                errorCode,
                std::generic_category(),
                "socketpair failed"
            );
        }

        net::UniqueFd sender{fds[0]};
        net::UniqueFd receiver{fds[1]};

        net::SetNonBlocking(receiver.Get());

        net::EpollPoller poller;
        poller.Add(receiver.Get(), EPOLLIN);

        // 当前没有数据，等待一小段时间后返回空集合。
        const auto initial = poller.Wait(100);
        std::cout << "发送前事件数: " << initial.size() << '\n';

        // 演示仅发送本地单字节，检查实际发送结果。
        if (::send(sender.Get(), "X", 1, MSG_NOSIGNAL) != 1) {
            throw std::runtime_error("demo send failed");
        }

        const auto ready = poller.Wait(1000);
        std::cout << "发送后事件数：" << ready.size() << '\n';

        // 故意先不读取。
        // LT 模式下，数据还在，下一次仍会报告可读。
        const auto stillReady = poller.Wait(0);
        std::cout << "未读取时再次检查：" << stillReady.size() << '\n';

        for (const auto& event : ready) {
            // & 是按位与：检查事件集合中是否包含 EPOLLIN。
            // 不能写成 == EPOLLIN，因为可能同时存在其他事件位。
            if ((event.events & EPOLLIN) != 0)  {
                const auto result = net::TryReceive(event.data.fd);

                if (result.status == net::ReadStatus::kData) {
                    std::cout << "收到数据：" << result.data << '\n';
                }
            }
        }

        const auto drained = poller.Wait(0);
        std::cout << "读完后事件数: " << drained.size() << '\n';

        // 先取消关注，再关闭被关注对象，明确清理顺序。
        poller.Remove(receiver.Get());
        receiver.Close();
    } catch (const std::exception& error) {
        std::cerr << "Demo failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}