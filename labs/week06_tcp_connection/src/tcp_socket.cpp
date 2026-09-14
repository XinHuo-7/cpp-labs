#include "tcp_socket.h"

#include <cerrno>
#include <system_error>

#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>

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
    Close();
}

int TcpSocket::GetFd() const noexcept {
    return fd_;
}

void TcpSocket::BindLoopback(std::uint16_t port) {
    RequireState(SocketState::kCreated);
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
    state_ = SocketState::kBound;
}

void TcpSocket::Listen(int backlog) {
    RequireState(SocketState::kBound);
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
    state_ = SocketState::kListening;
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

TcpSocket::TcpSocket(int acceptedFd) noexcept : fd_(acceptedFd), state_(SocketState::kConnected) {

}

void TcpSocket::ConnectLoopback(std::uint16_t port) {
    RequireState(SocketState::kCreated);
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
    state_ = SocketState::kConnected;
}

TcpSocket TcpSocket::Accept() {
    RequireState(SocketState::kListening);
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
    RequireState(SocketState::kConnected);
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
            
            Abort();

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "send failed"
            );
        }

        if (result == 0) {
            Abort();
            throw std::runtime_error("send made no progress");
        }

        sent += static_cast<std::size_t>(result);
    }
    
}

std::string TcpSocket::ReceiveExact(std::size_t byteCount) {
    RequireState(SocketState::kConnected);
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

            if (errorCode == EAGAIN || errorCode == EWOULDBLOCK) {
                CloseWithState(SocketState::kTimedOut);
                throw std::system_error(
                    errorCode,
                    std::generic_category(),
                    "receive timeout"
                );
            }
            
            Abort();
            throw std::system_error {
                errorCode,
                std::generic_category(),
                "recv failed"
            };
        }

        if (result == 0) {
            CloseWithState(SocketState::kPeerClosed);
            throw std::runtime_error("peer ended sending before all expected bytes arrived");
        }

        received += static_cast<std::size_t>(result);
    }
    return data;
}

void TcpSocket::SetReceiveTimeout(int timeoutMs) {
    if (timeoutMs <= 0) {
        throw std::invalid_argument("receive timeout must be positive");
    }

    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    const int result = ::setsockopt (
        fd_,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)
    );

    if (result == -1) {
        const int errorCode = errno;

        throw std::system_error(
            errorCode,
            std::generic_category(),
            "setting receive timeout failed"
        );
    }
}

const char* ToText(SocketState state) noexcept {
    switch (state) {
    case SocketState::kCreated:
        return "CREATED";
    case SocketState::kBound:
        return "BOUND";
    case SocketState::kListening:
        return "LISTENING";
    case SocketState::kConnected:
        return "CONNECTED";
    case SocketState::kPeerClosed:
        return "PEER_CLOSED";
    case SocketState::kTimedOut:
        return "TIMED_OUT";
    case SocketState::kFailed:
        return "FAILED";
    case SocketState::kClosed:
        return "CLOSED";
    }

    return "UNKNOWN";
}

SocketState TcpSocket::GetState() const noexcept {
    return state_;
}

void TcpSocket::RequireState(SocketState expected) const {
    if (state_ != expected) {
        throw std::logic_error(std::string("unexpected socket state: ") + ToText(state_));
    }
}

void TcpSocket::CloseWithState(SocketState finalState) noexcept {
    const int oldFd = fd_;
    fd_ = -1;
    state_ = finalState;
    if (oldFd >= 0) {
        ::close(oldFd);
    }
}

void TcpSocket::Close() noexcept {
    if (fd_ >= 0) {
        CloseWithState(SocketState::kClosed);
    }
}

void TcpSocket::Abort() noexcept {
    if (fd_ >= 0) {
        CloseWithState(SocketState::kFailed);
    }
}