#include "tcp_socket.h"
#include "message_protocol.h"

#include <cerrno>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include <fcntl.h>
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

    void RequireFdClosed(int savedFd) {
        errno = 0;

        const int result = ::fcntl(savedFd, F_GETFD);
        const int errorCode = errno;

        Require(result == -1, "Descriptor must be closed");
        Require(errorCode == EBADF, "Expected EBADF");
    }

    void RequireCannotReuse(TcpSocket& socket) {
        const auto previousState = socket.GetState();

        bool sendRejected = false;
        try {
            socket.SendAll("X");
        } catch (const std::logic_error&) {
            sendRejected = true;
        }

        bool recevieRejected = false;
        try {
            (void)socket.ReceiveExact(1);
        } catch (const std::logic_error&) {
            recevieRejected = true;
        }

        Require(sendRejected, "Send after closure must fail");
        Require(recevieRejected, "Receive after closure must fail");

        // 重复关闭，不覆盖原来的结束原因。
        socket.Close();
        socket.Close();
        Require(socket.GetState() == previousState, "Terminal state must be preserved");
    }

    void TestNormalLifecycle() {
        TcpSocket listener;
        Require(listener.GetState() == SocketState::kCreated, "Expected CREATED");

        listener.BindLoopback(0);
        Require(listener.GetState() == SocketState::kBound, "Expected BOUND");

        listener.Listen();
        Require(listener.GetState() == SocketState::kListening, "Expected LISTENING");

        TcpSocket client;
        client.ConnectLoopback(listener.GetLocalPort());
        TcpSocket connection = listener.Accept();
        Require(client.GetState() == SocketState::kConnected, "Client must be connected");
        Require(connection.GetState() == SocketState::kConnected, "Accept must be connected");

        client.SetReceiveTimeout(1000);
        connection.SetReceiveTimeout(1000);

        // 本地超长参数拒绝，但连接保持可用。
        bool oversizedRejected = false;

        try {
            protocol::SendMessage(client, std::string(protocol::kMaxMessageSize + 1, 'X'));
        } catch (const std::length_error&) {
            oversizedRejected = true;
        }

        Require(oversizedRejected, "Oversized send must fail");
        Require(client.GetState() == SocketState::kConnected, "Local validation must not close the connection");

        protocol::SendMessage(client, "PING");
        Require(protocol::ReceiveMessage(connection) == "PING", "Request mismatch");

        protocol::SendMessage(connection, "PONG");
        Require(protocol::ReceiveMessage(client) == "PONG", "Response mismatch");

        const int saveFd = connection.GetFd();
        connection.Close();
        RequireFdClosed(saveFd);
        Require(connection.GetFd() == -1, "Expected invalid fd");
        Require(connection.GetState() == SocketState::kClosed, "Expected CLOSED");
        RequireCannotReuse(connection);

        Require(listener.GetState() == SocketState::kListening, "Closing a connection must not change the listener");

        Require(::fcntl(listener.GetFd(), F_GETFD) != -1, "Listener descirptor must remain open");

        std::cout << "[normal] CONNECTED -> CLOSED\n";
    }

    void TestReceiveFailure(const char* label, std::string_view bytes, bool endSending, SocketState expectedState) {
        TcpSocket listener;
        TcpSocket client;
        PrepareConnection(listener, client);
        TcpSocket connection = listener.Accept();
        connection.SetReceiveTimeout(200);
        client.SendAll(bytes);
        if (endSending) {
            Require(::shutdown(client.GetFd(), SHUT_WR) == 0, "shutdown failed");
        }

        const int saveFd = connection.GetFd();
        bool caught = false;
        std::string reason;

        try {
            (void)protocol::ReceiveMessage(connection);
        } catch (const std::system_error& error) {
            const int code = error.code().value();

            if (expectedState != SocketState::kTimedOut || (code != EAGAIN && code != EWOULDBLOCK)) {
                throw;
            }
            caught = true;
            reason = error.what();
        } catch (const std::length_error& error) {
            if (expectedState != SocketState::kFailed) {
                throw;
            }
            caught = true;
            reason = error.what();
        } catch (const std::runtime_error& error) {
            if (expectedState != SocketState::kPeerClosed || std::string(error.what()) != "peer ended sending before all expected bytes arrived") {
                throw;
            }
            caught = true;
            reason = error.what();
        }
        Require(caught, "Expected receive failure");
        Require(connection.GetState() == expectedState, "Unexpected terminal state");
        Require(connection.GetFd() == -1, "Socket must release fd");
        RequireFdClosed(saveFd);
        RequireCannotReuse(connection);
        std::cout << '[' << label << ']' << " CONNECTED -> " << ToText(connection.GetState()) << " | fd=" << saveFd
                  << " | reason: " << reason << '\n';
    }

}

// 验证点状态正常迁移。
/*
对端结束发送与半帧截断。
无数据、半个消息头、半个正文的超时。
非法消息长度。
失败后描述符确实释放。
失败后收发被拒绝。
重复关闭安全。
已接收连接关闭不影响监听 socket。
本地发送参数错误不会误关连接。
*/

int main() {
    try {
        TestNormalLifecycle();
        TestReceiveFailure("peer-end", "", true, SocketState::kPeerClosed);
        TestReceiveFailure("truncated-header", std::string("\0\0", 2), true, SocketState::kPeerClosed);
        TestReceiveFailure("truncated-body", std::string("\0\0\0\4AB", 6), true, SocketState::kPeerClosed);
        TestReceiveFailure("idle-timeout", "", false, SocketState::kTimedOut);
        TestReceiveFailure("header-timeout", std::string("\0\0", 2), false, SocketState::kTimedOut);
        TestReceiveFailure("body-timeout", std::string("\0\0\0\4AB", 6), false, SocketState::kTimedOut);
        // 网络字节序的1025, 超过协议上限
        TestReceiveFailure("oversized-header", std::string("\0\0\4\1", 4), false, SocketState::kFailed);
        std::cout << "All connection lifecycle tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}