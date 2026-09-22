#include "epoll_poller.h"
#include "io_utils.h"

#include <cerrno>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>

#include <fcntl.h>
#include <sys/socket.h>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// 只用于测试，继续使用 Day1 的本地流式 socket。
struct LocalPair {
    net::UniqueFd sender;
    net::UniqueFd receiver;

    LocalPair() {
        int fds[2]{-1, -1};

        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
            const int errorCode = errno;

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "socketpair failed"
            );
        }

        sender = net::UniqueFd{fds[0]};
        receiver = net::UniqueFd{fds[1]};

        net::SetNonBlocking(receiver.Get());
    }
};

void SendSmall(int fd, std::string_view data) {
    // 仅用于本测试中的 1～2 字节数据，不是通用发送封装。
    const ssize_t result = ::send(
        fd,
        data.data(),
        data.size(),
        MSG_NOSIGNAL
    );

    Require(
        result == static_cast<ssize_t>(data.size()),
        "Test send failed"
    );
}

void RequireReadable(
    const std::vector<epoll_event>& events,
    int fd
) {
    bool found = false;

    for (const auto& event : events) {
        if (event.data.fd == fd &&
            (event.events & EPOLLIN) != 0) {
            found = true;
        }
    }

    Require(found, "Expected readable event for this fd");
}

void TestEpollLifetime() {
    LocalPair pair;
    int savedEpollFd = -1;

    {
        net::EpollPoller poller;
        savedEpollFd = poller.GetFd();

        const int flags = ::fcntl(savedEpollFd, F_GETFD);
        Require(flags != -1, "Epoll descriptor must be valid");

        Require(
            (flags & FD_CLOEXEC) != 0,
            "Epoll descriptor must have FD_CLOEXEC"
        );

        poller.Add(pair.receiver.Get());
    }

    errno = 0;
    const int result = ::fcntl(savedEpollFd, F_GETFD);
    const int errorCode = errno;

    Require(result == -1, "Epoll descriptor must be closed");
    Require(errorCode == EBADF, "Expected EBADF");

    // 销毁 epoll 对象不能顺便关闭被关注的 socket。
    Require(
        ::fcntl(pair.receiver.Get(), F_GETFD) != -1,
        "Watched socket must remain open"
    );
}

void TestLevelTriggeredAndDrain() {
    LocalPair pair;
    net::EpollPoller poller;

    poller.Add(pair.receiver.Get());

    Require(poller.Wait(0).empty(), "Expected no initial event");
    Require(poller.Wait(20).empty(), "Expected wait timeout");

    SendSmall(pair.sender.Get(), "AB");

    RequireReadable(poller.Wait(1000), pair.receiver.Get());

    // 不读取，仍应继续报告可读。
    RequireReadable(poller.Wait(0), pair.receiver.Get());

    // 故意只读一个字节，验证“部分读取后仍然通知”。
    char first = '\0';
    Require(
        ::recv(pair.receiver.Get(), &first, 1, 0) == 1,
        "Reading first byte failed"
    );
    Require(first == 'A', "Expected A");

    RequireReadable(poller.Wait(0), pair.receiver.Get());

    const auto remaining = net::TryReceive(pair.receiver.Get());

    Require(
        remaining.status == net::ReadStatus::kData,
        "Expected remaining data"
    );
    Require(remaining.data == "B", "Expected B");

    Require(
        poller.Wait(0).empty(),
        "Drained socket should no longer be readable"
    );
}

void TestRemoveAndAddAgain() {
    LocalPair pair;
    net::EpollPoller poller;

    poller.Add(pair.receiver.Get());
    poller.Remove(pair.receiver.Get());

    SendSmall(pair.sender.Get(), "X");

    Require(
        poller.Wait(0).empty(),
        "Removed socket must not produce events here"
    );

    Require(
        ::fcntl(pair.receiver.Get(), F_GETFD) != -1,
        "Remove must not close the socket"
    );

    // 数据已经存在，再次注册后也应报告可读。
    poller.Add(pair.receiver.Get());
    RequireReadable(poller.Wait(1000), pair.receiver.Get());

    const auto result = net::TryReceive(pair.receiver.Get());
    Require(result.data == "X", "Data must remain available");
}

void TestDataBeforeEof() {
    LocalPair pair;
    net::EpollPoller poller;

    poller.Add(pair.receiver.Get());

    SendSmall(pair.sender.Get(), "Z");

    Require(
        ::shutdown(pair.sender.Get(), SHUT_WR) == 0,
        "shutdown failed"
    );

    RequireReadable(poller.Wait(1000), pair.receiver.Get());

    // 对端结束发送后，之前发送的数据仍然必须先读完。
    const auto data = net::TryReceive(pair.receiver.Get());
    Require(data.status == net::ReadStatus::kData, "Expected DATA");
    Require(data.data == "Z", "Expected Z before EOF");

    // EOF 也会使 socket 呈现可读状态。
    RequireReadable(poller.Wait(1000), pair.receiver.Get());

    const auto eof = net::TryReceive(pair.receiver.Get());
    Require(
        eof.status == net::ReadStatus::kPeerClosed,
        "Expected EOF after queued data"
    );

    // EOF 条件不会因读取一次就消失，因此取消关注。
    poller.Remove(pair.receiver.Get());
    Require(poller.Wait(0).empty(), "Expected no events after removal");
}

void TestTwoSockets() {
    LocalPair first;
    LocalPair second;
    net::EpollPoller poller;

    poller.Add(first.receiver.Get());
    poller.Add(second.receiver.Get());

    SendSmall(first.sender.Get(), "A");
    SendSmall(second.sender.Get(), "B");

    bool sawFirst = false;
    bool sawSecond = false;

    // 不假设事件顺序，也不要求所有事件一定在同一批返回。
    for (int attempt = 0; attempt < 2 && !(sawFirst && sawSecond); ++attempt) {
        const auto events = poller.Wait(1000);

        for (const auto& event : events) {
            const auto result = net::TryReceive(event.data.fd);

            Require(
                result.status == net::ReadStatus::kData,
                "Expected data from a ready socket"
            );

            if (event.data.fd == first.receiver.Get()) {
                Require(result.data == "A", "First socket mismatch");
                sawFirst = true;
            } else if (event.data.fd == second.receiver.Get()) {
                Require(result.data == "B", "Second socket mismatch");
                sawSecond = true;
            } else {
                Require(false, "Unexpected descriptor");
            }
        }
    }

    Require(sawFirst && sawSecond, "Both sockets must be observed");
}

void TestInvalidOperations() {
    LocalPair pair;
    net::EpollPoller poller;

    poller.Add(pair.receiver.Get());

    bool duplicateRejected = false;

    try {
        // 同一个 fd 重复 ADD，应报告 EEXIST。
        poller.Add(pair.receiver.Get());
    } catch (const std::system_error& error) {
        if (error.code().value() != EEXIST) {
            throw;
        }
        duplicateRejected = true;
    }

    Require(duplicateRejected, "Duplicate ADD must be rejected");

    bool timeoutRejected = false;

    try {
        (void)poller.Wait(-2);
    } catch (const std::invalid_argument&) {
        timeoutRejected = true;
    }

    Require(timeoutRejected, "Invalid timeout must be rejected");
}

} // namespace

int main() {
    try {
        TestEpollLifetime();
        TestLevelTriggeredAndDrain();
        TestRemoveAndAddAgain();
        TestDataBeforeEof();
        TestTwoSockets();
        TestInvalidOperations();

        std::cout << "All epoll poller tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}