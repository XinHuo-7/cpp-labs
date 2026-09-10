#include "tcp_socket.h"

#include <cerrno>
#include <system_error>

#include <sys/socket.h>
#include <unistd.h>

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