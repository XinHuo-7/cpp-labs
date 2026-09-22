#include "epoll_poller.h"
#include "timer_fd.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
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

std::uint64_t WaitForExpiration(net::EpollPoller& poller, net::TimerFd& timer) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);

    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto events = poller.Wait(20);
        for (const auto& event : events) {
            if (event.data.fd == timer.GetFd() && (event.events & EPOLLIN) != 0) {
                const auto count = timer.Consume();
                if (count > 0) {
                    return count;
                }
            }
        }
    }
    throw std::runtime_error("timer did not expire");
}

void TestInitialStateAndLifetime() {
    int savedFd = -1;
    {
        net::TimerFd timer;
        savedFd = timer.GetFd();
        Require(savedFd >= 0, "invalid timer fd");

        // 尚未启动，无到期次数，非阻塞读取应返回 0。
        Require(timer.Consume() == 0, "new timer should be idle");

        const int statusFlags = ::fcntl(savedFd, F_GETFL);
        Require(statusFlags != -1, "F_GETFL failed");
        Require(
            (statusFlags & O_NONBLOCK) != 0,
            "timer must be nonblocking");
        const int descriptorFlags = ::fcntl(savedFd, F_GETFD);
        Require(descriptorFlags != -1, "F_GETFD failed");
        Require(
            (descriptorFlags & FD_CLOEXEC) != 0,
            "timer must be close-on-exec");
    }
    // savedFd 只是编号副本。
    // TimerFd 析构后，内核里的这个描述符应已关闭。
    errno = 0;
    const int result = ::fcntl(savedFd, F_GETFD);

    Require(
        result == -1 && errno == EBADF,
        "timer fd was not closed");
}

void TestOneShot() {
    net::TimerFd timer;
    net::EpollPoller poller;

    poller.Add(timer.GetFd(), EPOLLIN);
    timer.Start(30);  // 30 毫秒后触发一次，不重复。

    Require(
        WaitForExpiration(poller, timer) == 1,
        "one-shot timer should expire once");

    // 已经消费，且没有后续重复计时。
    Require(timer.Consume() == 0, "expiration was not consumed");
    Require(poller.Wait(50).empty(), "one-shot timer repeated");
}

void TestPeriodicStopAndRestart() {
    net::TimerFd timer;
    net::EpollPoller poller;

    poller.Add(timer.GetFd(), EPOLLIN);
    timer.Start(20, 20);

    // 不断言每次恰好为 1：系统忙时可能累计多个到期次数。
    Require(
        WaitForExpiration(poller, timer) >= 1,
        "first periodic expiration missing");

    Require(
        WaitForExpiration(poller, timer) >= 1,
        "periodic timer did not repeat");

    timer.Stop();

    Require(timer.Consume() == 0, "stopped timer has pending count");
    Require(poller.Wait(60).empty(), "stopped timer still fires");

    // Stop 没有关闭 fd，因此可以重新启动。
    timer.Start(20);

    Require(
        WaitForExpiration(poller, timer) == 1,
        "timer did not restart");
}
void TestInvalidArguments() {
    net::TimerFd timer;

    bool rejected = false;

    try {
        timer.Start(0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    Require(rejected, "zero first delay should be rejected");

    rejected = false;

    try {
        timer.Start(10, -1);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    Require(rejected, "negative interval should be rejected");
}

} // namespace

int main() {
    try {
        TestInitialStateAndLifetime();
        TestOneShot();
        TestPeriodicStopAndRestart();
        TestInvalidArguments();

        std::cout << "All timer fd tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}