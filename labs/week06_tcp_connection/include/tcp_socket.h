#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

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
    void SendAll(std::string_view data);
    std::string ReceiveExact(std::size_t byteCount);

private:
    explicit TcpSocket(int acceptedFd) noexcept;

    int fd_{-1};

};