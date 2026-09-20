#include "tcp_server.h"

#include <cerrno>
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <system_error>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace {

// 测试辅助函数只在本文件使用，放入匿名命名空间。
void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

net::UniqueFd ConnectClient(std::uint16_t port) {
    // 测试客户端使用阻塞 socket。
    // 服务端监听与连接 socket 仍然是非阻塞的。
    net::UniqueFd client(
        ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0));

    if (!client.IsValid()) {
        throw std::system_error(
            errno, std::generic_category(), "client socket failed");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);

    if (::connect(
            client.Get(),
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) == -1) {
        throw std::system_error(
            errno, std::generic_category(), "connect failed");
    }

    // UniqueFd 不能复制，但可以通过返回值优化或移动返回。
    return client;
}

void SendAll(int fd, std::string_view data) {
    std::size_t sent = 0;

    while (sent < data.size()) {
        const auto result = ::send(
            fd,
            data.data() + sent,
            data.size() - sent,
            MSG_NOSIGNAL);

        // MSG_NOSIGNAL：对端关闭时用返回错误报告，
        // 避免 SIGPIPE 直接终止测试进程。
        if (result > 0) {
            sent += static_cast<std::size_t>(result);
            continue;
        }

        if (result == -1 && errno == EINTR) {
            continue;
        }

        if (result == -1) {
            throw std::system_error(
                errno, std::generic_category(), "send failed");
        }

        throw std::runtime_error("send made no progress");
    }
}

void EndSending(int fd) {
    // 关闭客户端的发送方向，让服务端最终读到 EOF。
    if (::shutdown(fd, SHUT_WR) == -1) {
        throw std::system_error(
            errno, std::generic_category(), "shutdown failed");
    }
}

// Predicate 可以是一个 lambda。
// 不用固定 sleep 猜测时序，而是在有期限的循环中检查结果。
template <typename Predicate>
void PumpUntil(net::TcpServer& server, Predicate done, const char* failureMessage) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);

    while (!done()) {
        Require(
            std::chrono::steady_clock::now() < deadline,
            failureMessage);

        server.RunOnce(10);
    }   
}

void TestIdleServer() {
    net::TcpServer server;
    Require(server.GetPort() != 0, "port was not assigned");

    // timeout 为 0，没有事件也应立即返回。
    server.RunOnce(0);
    Require(server.ConnectionCount() == 0, "unexpected connection");
    Require(server.ReceivedBytes() == 0, "unexpected data");
}

void TestMultipleClients() {
    net::TcpServer server;
    auto clientA = ConnectClient(server.GetPort());
    auto clientB = ConnectClient(server.GetPort());

    // [&]：lambda 通过引用访问当前函数中的 server。
    PumpUntil(
        server,
        [&] {return server.ConnectionCount() == 2;},
        "two clients were not accepted"
    );

    // 显式长度使中间的 '\0' 也被发送。
    SendAll(clientA.Get(), std::string_view("A\0B", 3));
    SendAll(clientB.Get(), "CD");

    PumpUntil(
        server,
        [&] { return server.ReceivedBytes() == 5; },
        "initial data was not received");
        
    // A 退出，不应导致 B 被关闭。
    EndSending(clientA.Get());
    PumpUntil(
        server,
        [&] { return server.ConnectionCount() == 1; },
        "client A was not removed");

    Require(server.ClosedCount() == 1, "wrong closed count");

    // 验证 B 仍然可以继续发送。
    SendAll(clientB.Get(), "E");

    PumpUntil(
        server,
        [&] {return server.ReceivedBytes() == 6; },
        "client B stopped working"
    );

    EndSending(clientB.Get());

    PumpUntil(
        server,
        [&] {return server.ConnectionCount() == 0;},
        "client B was not removed"
    );

    // 旧连接结束后，服务端必须仍然能接受新连接。

    auto clientC = ConnectClient(server.GetPort());

    // 先发送，再结束发送方向。
    // 服务端必须先收完 FG，不能看到关闭通知就直接丢弃数据。
    SendAll(clientC.Get(), "FG");
    EndSending(clientC.Get());

     PumpUntil(
        server,
        [&] {
            return server.AcceptedCount() == 3
                && server.ClosedCount() == 3
                && server.ConnectionCount() == 0;
        },
        "client C lifecycle did not finish");

    Require(server.ReceivedBytes() == 8, "data before EOF was lost");
}

void TestEmptyPeerClose() {
    net::TcpServer server;
    auto client = ConnectClient(server.GetPort());
    // 不发送数据，直接结束发送。
    EndSending(client.Get());
    PumpUntil(
        server,
        [&] { return server.ClosedCount() == 1; },
        "empty connection was not closed");
    Require(server.AcceptedCount() == 1, "wrong accepted count");
    Require(server.ConnectionCount() == 0, "connection leaked");
    Require(server.ReceivedBytes() == 0, "EOF was counted as data");
}

void TestTimerAndNetworkTogether() {
    // 使用较短的统计周期，让自动化测试快速完成。
    net::TcpServer server(0, 20);

    // 没有任何客户端时，也必须能收到定时事件。
    PumpUntil(
        server,
        [&server] { return server.TimerTicks() > 0; },
        "idle server did not process timer");

    Require(
        server.ConnectionCount() == 0,
        "timer created an unexpected connection");

    auto client = ConnectClient(server.GetPort());

    PumpUntil(
        server,
        [&server] { return server.ConnectionCount() == 1; },
        "client was not accepted");

    const auto previousTicks = server.TimerTicks();

    SendAll(client.Get(), "XYZ");

    // 同时验证网络数据被读取，以及定时器继续工作。
    PumpUntil(
        server,
        [&server, previousTicks] {
            return server.ReceivedBytes() == 3
                && server.TimerTicks() > previousTicks;
        },
        "timer and network did not both progress");

    // previousTicks 按值捕获：
    // 保存进入等待前的计数，用作比较基准。
    Require(
        server.ConnectionCount() == 1,
        "statistics timer unexpectedly closed client");

    EndSending(client.Get());

    PumpUntil(
        server,
        [&server] { return server.ConnectionCount() == 0; },
        "client was not cleaned up");
}


}

int main() {
    try {
        TestIdleServer();
        TestMultipleClients();
        TestEmptyPeerClose();
        TestTimerAndNetworkTogether();
        std::cout << "All Tcp server tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}