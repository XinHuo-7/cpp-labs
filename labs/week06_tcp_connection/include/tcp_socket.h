#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

enum class SocketState {
    kCreated,
    kBound,
    kListening,
    kConnected,
    kPeerClosed,
    kTimedOut,
    kFailed,
    kClosed
};

const char* ToText(SocketState state) noexcept;

class TcpSocket {
public:
    TcpSocket();
    ~TcpSocket() noexcept;

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    int GetFd() const noexcept;
    SocketState GetState() const noexcept;

    void BindLoopback(std::uint16_t port);
    void Listen(int backlog = 16);
    std::uint16_t GetLocalPort() const;

    void ConnectLoopback(std::uint16_t port);
    TcpSocket Accept();

    void SendAll(std::string_view data);
    std::string ReceiveExact(std::size_t byteCount);
    void SetReceiveTimeout(int timeoutMs);

    void Close() noexcept;
    void Abort() noexcept;

private:
    explicit TcpSocket(int acceptedFd) noexcept;

    void RequireState(SocketState expected) const;
    void CloseWithState(SocketState finalState) noexcept;

    int fd_{-1};
    SocketState state_{SocketState::kCreated};

};