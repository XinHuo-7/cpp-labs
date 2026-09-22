#pragma once

#include <string>
#include <cstddef>

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

struct DrainResult {
    // 本次连续读取累计得到的数据，不保证是一条完整消息。
    std::string data;

    // 可能在读到若干数据之后，又读到了 EOF。
    // 因此 data 非空与 peerClosed 为 true 可以同时成立。
    bool peerClosed{false};

};

// fd 必须已经设置为非阻塞。
// 读到 kWouldBlock 或 EOF 才正常返回。
//
// maxBytes 限制本次累计数据量，防止无限增长。
// 超过限制会抛异常；当前练习策略是放弃连接，不恢复读取进度。

DrainResult DrainReadable (int fd, std::size_t maxBytes = 64 * 1024);


} // namespace net