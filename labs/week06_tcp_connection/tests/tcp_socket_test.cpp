#include "tcp_socket.h"

#include <cerrno>
#include <exception>
#include <iostream>
#include <stdexcept>

#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace {
    void Require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }
}

void TestCreatesValidSocket() {
    TcpSocket socket;
    Require(socket.GetFd() >= 0, "Socket descriptor must be nonnegative");

    const int result = ::fcntl(socket.GetFd(), F_GETFD);

    Require(result != -1, "Socket descriptor must be open");
}

void TestSocketsHaveDifferentDescriptors() {
    TcpSocket first;
    TcpSocket second;

    Require(first.GetFd() != second.GetFd(), "Live sockets must have different descriptors");
}

void TestDestructorClosesSocket() {
    int saveFd = -1;
    {
        TcpSocket socket;
        saveFd = socket.GetFd();
    }
    errno = 0;
    const int result = ::fcntl(saveFd, F_GETFD);
    const int errorCode = errno;

    Require(result == -1, "Descriptor must be closed after desctrusction");

    Require(errorCode == EBADF, "Closed descriptor must report EBADF");
}

void TestBindAssignsPort() {
    TcpSocket socket;
    socket.BindLoopback(0);

    Require(socket.GetLocalPort() != 0, "Bind must assign a nonzero port");
}

void TestSocketStartsListening() {
    TcpSocket socket;
    socket.BindLoopback(0);

    int accepting = -1;
    socklen_t length = sizeof(accepting);
    
    int result = ::getsockopt(
        socket.GetFd(),
        SOL_SOCKET,
        SO_ACCEPTCONN,
        &accepting,
        &length
    );

    Require(result == 0, "Initial getsockopt failed");
    Require(accepting == 0, "Socket must not listen yet");

    socket.Listen();
    accepting = -1;
    length = sizeof(accepting);

    result = ::getsockopt(
        socket.GetFd(),
        SOL_SOCKET,
        SO_ACCEPTCONN,
        &accepting,
        &length
    );

    Require(result == 0, "Listening getsockopt failed");
    Require(accepting == 1, "Socket must be listening");
}

void TestRejectsInvalidBacklog() {
    TcpSocket socket;
    socket.BindLoopback(0);

    bool caught = false;
    try {
        socket.Listen(0);
    } catch (const std::invalid_argument&) {
        caught = true;
    }

    Require(caught, "Zero backlog must be rejected");
}

void RequirePeerPort(const TcpSocket& socket, std::uint16_t excptedPort) {
    sockaddr_in peer{};
    socklen_t length = sizeof(peer);
    const int result = ::getpeername(
        socket.GetFd(),
        reinterpret_cast<sockaddr*>(&peer),
        &length
    );

    Require(result == 0, "getpeername failed");
    Require(ntohs(peer.sin_port) == excptedPort, "Unexpected peer port");
}

void TestConnectAndAccept() {
    TcpSocket listener;
    listener.BindLoopback(0);
    listener.Listen();

    const auto serverPort = listener.GetLocalPort();

    TcpSocket client;
    client.ConnectLoopback(serverPort);

    TcpSocket connection = listener.Accept();
    Require(connection.GetFd() != listener.GetFd(), "Accepted socket must differ from listener");
    Require(connection.GetFd() != client.GetFd(), "Client and serer must have different descriptors");
    Require(connection.GetLocalPort() == serverPort, "Accepted socket must use the server port");
    RequirePeerPort(client, serverPort);
    RequirePeerPort(connection, client.GetLocalPort());
}

void TestAcceptedSocketClosesAutomatically() {
    TcpSocket listener;
    listener.BindLoopback(0);
    listener.Listen();

    TcpSocket client;
    client.ConnectLoopback(listener.GetLocalPort());

    int saveFd = -1;
    {
        TcpSocket connection = listener.Accept();
        saveFd = connection.GetFd();
    }

    errno = 0;
    const int result = ::fcntl(saveFd, F_GETFD);
    const int errorCode = errno;

    Require(result == -1, "Accepted descriptor must be closed");
    Require(errorCode == EBADF, "Expected EBADF");

    const int listenerResult = ::fcntl(listener.GetFd(), F_GETFD);

    Require(listenerResult != -1, "Listener must remain open");
}

void TestRejectsZeroDestinationPort() {
    TcpSocket client;
    bool caught = false;
    try {
        client.ConnectLoopback(0);
    } catch (const std::invalid_argument&) {
        caught = true;
    }

    Require(caught, "Zero destination port must be rejected");
}

void TestRequestAndResponse() {
    TcpSocket listener;
    listener.BindLoopback(0);
    listener.Listen();

    TcpSocket client;
    client.ConnectLoopback(listener.GetLocalPort());

    TcpSocket connection = listener.Accept();

    client.SendAll("PING");

    const auto request = connection.ReceiveExact(4);
    Require(request == "PING", "Request mismatch");

    connection.SendAll("PONG");
    const auto response = client.ReceiveExact(4);
    Require(response == "PONG", "Response mistmatch");

}

// 测试\0也能接受，不会把\0当作数据结束标志
void TestTransfersEmbeddedNull() {
    TcpSocket listener;
    listener.BindLoopback(0);
    listener.Listen();

    TcpSocket client;
    client.ConnectLoopback(listener.GetLocalPort());

    TcpSocket connection = listener.Accept();

    const std::string payload("A\0B", 3);
    client.SendAll(payload);

    const auto received = connection.ReceiveExact(payload.size());
    Require(received.size() == 3, "Payload length mismatch");
    Require(received == payload, "Payload content mismatch");
}

// 尚未收到约定长度，对端结束发送
void TestDetectsEarlyEndOfStream() {
    TcpSocket listener;
    listener.BindLoopback(0);
    listener.Listen();

    TcpSocket client;
    client.ConnectLoopback(listener.GetLocalPort());

    TcpSocket connection = listener.Accept();
    client.SendAll("AB");
    const int shutdownResult = ::shutdown(client.GetFd(), SHUT_WR);
    Require(shutdownResult == 0, "shutdown failed");

    bool caught = false;
    try {
        const auto data = connection.ReceiveExact(4);
        (void) data;
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "peer ended sending before all expected bytes arrived";
    }

    Require(caught, "Early end of stream must be detected");
}

int main() {
    try
    {
        TestCreatesValidSocket();
        TestSocketsHaveDifferentDescriptors();
        TestDestructorClosesSocket();
        TestBindAssignsPort();
        TestSocketStartsListening();
        TestRejectsInvalidBacklog();
        TestConnectAndAccept();
        TestAcceptedSocketClosesAutomatically();
        TestRejectsZeroDestinationPort();
        TestRequestAndResponse();
        TestTransfersEmbeddedNull();
        TestDestructorClosesSocket();
        std::cout << "ALL TCP socket tests passed\n";   
    }
    catch(const std::exception& error)
    {
        std::cerr << "Test failed" << error.what() << '\n';
        return 1;
    }
    
    return 0;
}