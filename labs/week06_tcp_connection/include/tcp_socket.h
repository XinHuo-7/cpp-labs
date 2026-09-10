#pragma once

class TcpSocket {
public:
    TcpSocket();
    ~TcpSocket() noexcept;

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket operator=(const TcpSocket&) = delete;

    int GetFd() const noexcept;

private:
    int fd_{-1};

};