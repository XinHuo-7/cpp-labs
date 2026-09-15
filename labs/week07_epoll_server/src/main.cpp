#include "io_utils.h"

#include <cerrno>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <system_error>

#include <sys/socket.h>

namespace {
    void PrintResult(const net::ReadResult& result) {
        switch (result.status)
        {
            case net::ReadStatus::kData:
                std::cout << "DATA: " << result.data << '\n';
                break;
            case net::ReadStatus::kWouldBlock:
                std::cout << "WOULD_BLOCK: 当前没有数据\n";
                break;
            case net::ReadStatus::kPeerClosed:
                std::cout << "PEER_CLOSED: 对端结束发送\n";
                break;
        }
    }
} // namespace

int main() {
    try {
        int fds[2]{-1, -1};
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0 ,fds) == -1) {
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

        // 1. 当前没有任何数据。
        PrintResult(net::TryReceive(receiver.Get()));

        // 2. 演示只发送一个字节，避免引入完整发送循环。
        if (::send(sender.Get(), "X", 1, MSG_NOSIGNAL) != 1) {
            throw std::runtime_error("demo send failed");
        }

        PrintResult(net::TryReceive(receiver.Get()));

        // 3. 数据已经读完，但连接仍然存在。
        PrintResult(net::TryReceive(receiver.Get()));

        // 4. 对端主动结束发送。
        if (::shutdown(sender.Get(), SHUT_WR) == -1) {
            const int errorCode = errno;

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "shutdown failed"
            );
        }

        PrintResult(net::TryReceive(receiver.Get()));
    } catch (const std::exception& error) {
        std::cerr << "Demo failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}