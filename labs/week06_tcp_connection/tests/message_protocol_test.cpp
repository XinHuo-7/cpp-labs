#include "message_protocol.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/socket.h>

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

    // 验证不同长度，空消息，含零字节和最大长度
    void TestMessageContents() {
        TcpSocket listener;
        TcpSocket client;
        PrepareConnection(listener, client);

        TcpSocket connection = listener.Accept();

        const std::vector<std::string> messages{
            "PING",
            "GET_PORT Ethernet0",
            "",
            std::string("A\0B", 3),
            std::string(protocol::kMaxMessageSize, 'x')
        };

        for (const auto& message : messages) {
            protocol::SendMessage(client, message);

            const auto received = protocol::ReceiveMessage(connection);

            Require(message == received, "Request mismatch");

            protocol::SendMessage(connection, received);

            const auto reply = protocol::ReceiveMessage(client);

            Require(message == reply, "Reply mismatch");

        }
    }

    // 验证两条连在一起的报文，每次只取一条
    void TestConsecutiveFrames() {
        TcpSocket listener;
        TcpSocket client;
        PrepareConnection(listener, client);
        TcpSocket connection = listener.Accept();

        const char wire[] = {
            0, 0, 0, 3, 'O', 'N', 'E',
            0, 0, 0, 3, 'T', 'W', 'O',
        };

        client.SendAll(std::string_view(wire, sizeof(wire)));

        const auto first = protocol::ReceiveMessage(connection);
        const auto second = protocol::ReceiveMessage(connection);

        Require(first == "ONE", "First message mismatch");
        Require(second == "TWO", "Second message mismatch");

    }

    // 发送超长消息时拒绝，并且不污染后续数据
    void TestRejectsOversizedSend() {
        TcpSocket listener;
        TcpSocket client;
        PrepareConnection(listener, client);

        TcpSocket connection = listener.Accept();

        const std::string oversized(protocol::kMaxMessageSize + 1, 'x');

        bool caught = false;
        try {
            protocol::SendMessage(client, oversized);       
        } catch (const std::length_error&){
            caught = true;
        }

        Require(caught, "Oversized send must be rejected");

        protocol::SendMessage(client, "OK");
        const auto received = protocol::ReceiveMessage(connection);
        Require(received == "OK", "Rejected send changed the stream");
    }

    // 对端发送非法长度头时，接收方也拒绝
    void TestRejectsOversizedHeader() {
        TcpSocket listener;
        TcpSocket client;
        PrepareConnection(listener, client);
        TcpSocket connection = listener.Accept();

        // 00 00 04 01:网络字节序1025，超过约定的1024
        const char header[] = {0, 0, 4, 1};
        client.SendAll(std::string_view(header, sizeof(header)));

        bool caught = false;

        try {
            const auto message = protocol::ReceiveMessage(connection);
            (void)message;
        } catch (const std::length_error&) {
            caught = true;
        }

        Require(caught, "Oversized header must be rejected");
    }

    // 长度头或正文没收完，对端就结束发送
    void TestRejectsTruncatedFrames() {
        const std::vector<std::string> incompleteFrames {
            std::string("\0\0", 2),        // 头只有2字节
            std::string("\0\0\0\4AB", 6)   // 正文4字节，实际只有AB
        };
        
        for (const auto& frame : incompleteFrames) {
            TcpSocket listener;
            TcpSocket client;
            PrepareConnection(listener, client);
            TcpSocket connection = listener.Accept();

            client.SendAll(frame);

            const int result = ::shutdown(client.GetFd(), SHUT_WR);
            Require(result == 0, "shutdown failed");

            bool caught = false;
            try {
                const auto message = protocol::ReceiveMessage(connection);
                (void) message;
            } catch (const std::runtime_error& error) {
                caught = std::string(error.what()) == "peer ended sending before all expected bytes arrived";
            }
            Require(caught, "Truncated frame must be rejected");
        }
    }
}

int main() {
    try {
        TestMessageContents();
        TestConsecutiveFrames();
        TestRejectsOversizedSend();
        TestRejectsOversizedHeader();
        TestRejectsTruncatedFrames();

        std::cout << "All message protocol tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: "
                  << error.what() << '\n';
        return 1;
    }

    return 0;
}