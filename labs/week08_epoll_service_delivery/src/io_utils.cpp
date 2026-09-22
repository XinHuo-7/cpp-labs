#include "io_utils.h"

#include <cerrno>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>

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

DrainResult DrainReadable(int fd, std::size_t maxBytes) {
    if (maxBytes == 0) {
        throw std::invalid_argument("maxBytes must be positive");
    }

    DrainResult result;

    for (;;) {
        const auto chunk = TryReceive(fd);

        if (chunk.status == ReadStatus::kData) {
            // 使用减法检查剩余空间，避免先做加法产生溢出。
            // result.data.size() 始终不超过 maxBytes
            if (chunk.data.size() > maxBytes - result.data.size()) {
                throw std::length_error("receive batch exceeds limit");
            }       
            // append 保留全部字节，包括字符串内部的 '\0'。    
            result.data.append(chunk.data);
            // 收到数据不代表已经读空，继续尝试。
            continue;
        }

        if (chunk.status == ReadStatus::kWouldBlock) {
            // 底层 recv 已返回 EAGAIN/EWOULDBLOCK。
            // 当前暂时没有更多数据，可以回到 epoll 等待。
            return result;
        }

        // 剩下的是 kPeerClosed。
        // 保留前面已收到的数据，同时告诉调用方观察到了 EOF。
        result.peerClosed = true;
        return result;
    }

}

}


