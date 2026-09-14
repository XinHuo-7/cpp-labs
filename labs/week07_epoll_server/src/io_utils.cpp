#include "io_utils.h"

#include <cerrno>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace net {
    UniqueFd::UniqueFd(int fd) noexcept : fd_(fd) {}
    UniqueFd::~UniqueFd() noexcept {
        Close();
    }

    UniqueFd::UniqueFd(UniqueFd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {

    }

    UniqueFd& UniqueFd::operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            // 当前对象可能已经持有资源，先释放。
            Close();

            // 接管对方资源，同时把对方置为空。
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    int UniqueFd::Get() const noexcept {
        return fd_;
    }

    bool UniqueFd::IsValid() const noexcept {
        return fd_ >= 0;
    }

    void UniqueFd::Close() noexcept {
        const int oldFd = std::exchange(fd_, -1);

        if (oldFd >= 0) {
            ::close(oldFd);
        }
    }

    void SetNonBlocking(int fd) {
        const int flags = ::fcntl(fd, F_GETFL);

        if (flags == -1) {
            const int errorCode = errno;

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "F_GETFL failed"
            );
        }

        if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            const int errorCode = errno;

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "F_SETFL failed"
            );
        }
    }

ReadResult TryReceive(int fd) {
    char buffer[4096];

    for(;;) {
        const ssize_t result = ::recv(
            fd,
            buffer,
            sizeof(buffer),
            0
        );

        if (result > 0) {
            return {ReadStatus::kData, std::string(buffer, static_cast<std::size_t>(result))};
        }

        if (result == 0) {
            return {ReadStatus::kPeerClosed, {}};
        }

        const int errorCode = errno;

        if (errorCode == EINTR) {
            continue;
        }

        if (errorCode == EAGAIN || errorCode == EWOULDBLOCK) {
            return {ReadStatus::kWouldBlock, {}};
        }

        throw std::system_error(
            errorCode,
            std::generic_category(),
            "recv failed"
        );
    }
}
}


