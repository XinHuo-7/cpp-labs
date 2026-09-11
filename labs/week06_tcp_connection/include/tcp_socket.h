#pragma once

#include <cstdint>

class TcpSocket {
public:
    TcpSocket();
    ~TcpSocket() noexcept;

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    int GetFd() const noexcept;

    void BindLoopback(std::uint16_t port);
    void Listen(int backlog = 16);
    std::uint16_t GetLocalPort() const;

    void ConnectLoopback(std::uint16_t port);
    TcpSocket Accept();

private:
    explicit TcpSocket(int acceptedFd) noexcept;

    int fd_{-1};

};