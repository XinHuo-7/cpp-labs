#include "tcp_socket.h"

#include <cerrno>
#include <exception>
#include <iostream>
#include <stdexcept>

#include <fcntl.h>

namespace {
    void Require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }
}

void TestCreatesValidSocket() {
    TcpSocket socket;
    Require(socket.GetFd() >= 0, "Socket descriptor must be nonnegative");

    const int result = ::fcntl(socket.GetFd(), F_GETFD);

    Require(result != -1, "Socket descriptor must be open");
}

void TestSocketsHaveDifferentDescriptors() {
    TcpSocket first;
    TcpSocket second;

    Require(first.GetFd() != second.GetFd(), "Live sockets must have different descriptors");
}

void TestDestructorClosesSocket() {
    int saveFd = -1;
    {
        TcpSocket socket;
        saveFd = socket.GetFd();
    }
    errno = 0;
    const int result = ::fcntl(saveFd, F_GETFD);
    const int errorCode = errno;

    Require(result == -1, "Descriptor must be closed after desctrusction");

    Require(errorCode == EBADF, "Closed descriptor must report EBADF");
}

int main() {
    try
    {
        TestCreatesValidSocket();
        TestSocketsHaveDifferentDescriptors();
        TestDestructorClosesSocket();
        std::cout << "ALL TCP socket tests passed\n";   
    }
    catch(const std::exception& error)
    {
        std::cerr << "Test failed" << error.what() << '\n';
        return 1;
    }
    
    return 0;
}