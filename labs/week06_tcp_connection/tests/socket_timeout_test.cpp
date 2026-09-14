#include "tcp_socket.h"
#include "message_protocol.h"

#include <cerrno>
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {
    void Require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    void PrepareConnection(TcpSocket& listener, TcpSocket& client) {
        listener.BindLoopback(0);
        listener.Listen();
        client.ConnectLoopback(listener.GetLocalPort());
    }

    bool IsReceiveTimeout(const std::system_error& error) {
        const int code = error.code().value();
        return code == EAGAIN || code == EWOULDBLOCK;
    }

    // 测试一，拒绝零和负数
    void TestRejectsInvalidTimeout() {
        TcpSocket socket;

        for (const int timeoutMs : {0, -1}) {
            bool caught = false;
            try {
                socket.SetReceiveTimeout(timeoutMs);
            } catch (const std::invalid_argument&) {
                caught = true;
            }

            Require(caught, "Invalid timeout must be rejected");
        }
    }

    // 测试二: 设置超时后，完整消息仍然能正常收发
    // 同时验证正文仍然合法
    void TestMessagesWithTimeout() {
        TcpSocket listener;
        TcpSocket client;

        PrepareConnection(listener, client);

        TcpSocket connection = listener.Accept();

        connection.SetReceiveTimeout(1000);
        client.SetReceiveTimeout(1000);

        const std::vector<std::string> messages {"PING", "", std::string("A\0B", 3)};
        for (const auto& message : messages) {

            protocol::SendMessage(client, message);
            const auto request = protocol::ReceiveMessage(connection);
            Require(request == message, "Request mismatch");

            protocol::SendMessage(connection, message);
            const auto response = protocol::ReceiveMessage(client);
            Require(response == message, "Response mismatch");
        }
    }

    // 测试三，对端不再提供数据，但仍保持连接
    // 每种场景都使用一条新连接
    void TestStalledFramesTimeOut() {
        const std::vector<std::string> prefixes{
            "",
            std::string("\0\0", 2),
            std::string("\0\0\0\4AB", 6)
        };

        for (std::size_t i = 0; i < prefixes.size(); ++i) {
            TcpSocket listener;
            TcpSocket client;
            PrepareConnection(listener, client);

            TcpSocket connection = listener.Accept();
            connection.SetReceiveTimeout(200);
            client.SendAll(prefixes[i]);

            // 故意不shutdown，不关闭client
            const auto start = std::chrono::steady_clock::now();
            bool timeOut = false;

            try {
                (void)protocol::ReceiveMessage(connection);
            } catch (const std::system_error& error) {
                if (!IsReceiveTimeout(error)) {
                    throw;
                }
                timeOut = true;
            }

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
            Require(timeOut, "Stalled frame must time out");

            // 不要求精确等于200ms，避免依赖精确调度
            Require(elapsed.count() >= 100, "Receive returned unexpectedly early"); 

            std::cout << "[timeout case " << i + 1 << ']' <<elapsed.count() << " ms\n";
            // 当前消息可能已被部分读取，不继续复用这条连接。
            // 本轮结束后，由析构函数关闭。
        }
        
    }
}

int main() {
    try {
        TestRejectsInvalidTimeout();
        TestMessagesWithTimeout();
        TestStalledFramesTimeOut();

        std::cout << "All socket timeout tests passed\n";
    } catch (const std::exception& error) {
        std::cerr
            << "Test failed: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}