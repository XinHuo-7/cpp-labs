#include "tcp_socket.h"

#include <cerrno>
#include <system_error>

#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <arpa/inet.h>
#include <netinet/in.h>

TcpSocket::TcpSocket() {
    fd_ = :: socket(AF_INET, SOCK_STREAM, 0);

    if (fd_ == -1) {
        const int errorCode = errno;

        throw std::system_error(
            errorCode,
            std::generic_category(),
            "socket creation failed"
        );
    }
}

TcpSocket::~TcpSocket() noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

int TcpSocket::GetFd() const noexcept {
    return fd_;
}

void TcpSocket::BindLoopback(std::uint16_t port) {
    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    const int result = :: bind(
        fd_, 
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)
    );

    if (result == -1) {
        const int errorCode = errno;

        throw std::system_error(
            errorCode,
            std::generic_category(),
            "bind failed"
        );
    }
}

void TcpSocket::Listen(int backlog) {
    if (backlog <= 0) {
        throw std::invalid_argument("backlog must be positive");
    }

    if(::listen(fd_, backlog) == -1) {
        const int errorCode = errno;

        throw std::system_error(
            errorCode,
            std::generic_category(),
            "listen failed"
        );
    }
}

std::uint16_t TcpSocket::GetLocalPort() const {
    sockaddr_in address{};
    socklen_t addressLength = sizeof(address);

    const int result = ::getsockname(
        fd_,
        reinterpret_cast<sockaddr*>(&address),
        &addressLength
    );

    if (result == -1) {
        const int errorCode = errno;
        
        throw std::system_error(
            errorCode,
            std::generic_category(),
            "getsockname failed"
        );
    }

    return ntohs(address.sin_port);
}

TcpSocket::TcpSocket(int acceptedFd) noexcept : fd_(acceptedFd) {

}

void TcpSocket::ConnectLoopback(std::uint16_t port) {
    if (port == 0) {
        throw std::invalid_argument("destination port must be nonzero");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    const int result = ::connect(
        fd_,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)
    );

    if (result == -1) {
        const int errorCode = errno;

        throw std::system_error(
            errorCode,
            std::generic_category(),
            "connect failed"
        );
    }
}

TcpSocket TcpSocket::Accept() {
    const int acceptedFd = ::accept(
        fd_,
        nullptr,
        nullptr
    );

    if (acceptedFd == -1) {
        const int errorCode = errno;

        throw std::system_error (
            errorCode,
            std::generic_category(),
            "accept failed"
        );
    }

    return TcpSocket{acceptedFd};
}

void TcpSocket::SendAll(std::string_view data) {
    std::size_t sent = 0;

    while (sent < data.size())
    {
        const ssize_t result = ::send(
            fd_,
            data.data() + sent,  // 从尚未发送的位置开始
            data.size() - sent,  // 还剩多少字节需要发送
            MSG_NOSIGNAL
        );

        if (result == -1) {
            const int errorCode = errno;

            if (errorCode == EINTR) {
                continue;
            }   

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "send failed"
            );
        }

        if (result == 0) {
            throw std::runtime_error("send made no progress");
        }

        sent += static_cast<std::size_t>(result);
    }
    
}

std::string TcpSocket::ReceiveExact(std::size_t byteCount) {
    std::string data(byteCount, '\0');
    std::size_t received = 0;
    while (received < byteCount)
    {
        const ssize_t result = ::recv(
            fd_,
            data.data() + received,
            byteCount - received,
            0
        );

        if (result == -1) {
            const int errorCode =errno;

            if (errorCode == EINTR) {
                continue;
            }

            throw std::system_error {
                errorCode,
                std::generic_category(),
                "recv failed"
            };
        }

        if (result == 0) {
            throw std::runtime_error("peer ended sending before all expected bytes arrived");
        }

        received += static_cast<std::size_t>(result);
    }
    return data;
}