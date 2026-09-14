#pragma once

#include <string>

namespace net {

// 独占一个文字描述符，只管理资源，不管理TCP连接状态。
class UniqueFd {
    public:
        UniqueFd() noexcept = default;
        explicit UniqueFd(int fd) noexcept;
        ~UniqueFd() noexcept;

        UniqueFd(const UniqueFd&) = delete;
        UniqueFd& operator=(const UniqueFd&) = delete;

        UniqueFd(UniqueFd&& other) noexcept;
        UniqueFd& operator=(UniqueFd&& other) noexcept;

        int Get() const noexcept;
        bool IsValid() const noexcept;
        void Close() noexcept;
    private:
        int fd_{-1};
};

void SetNonBlocking(int fd);

enum class ReadStatus {
    kData,
    kWouldBlock,
    kPeerClosed
};

struct ReadResult
{
    ReadStatus status;
    std::string data;
};

// 调用前必须确保 fd 是非阻塞的流式 socket。
ReadResult TryReceive(int fd);

} // namespace net