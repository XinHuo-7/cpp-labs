#include "timer_fd.h"

#include <cerrno>
#include <stdexcept>
#include <system_error>

#include <sys/timerfd.h>
#include <unistd.h>

namespace {
// 仅在本文件中使用的转换辅助函数。
timespec ToTimespec(int milliseconds) {
    timespec result{};

    result.tv_sec = milliseconds / 1000;
    // timespec 使用纳秒，不是之前 timeval 使用的微秒。
    // 1 毫秒 = 1,000,000 纳秒。
    result.tv_nsec = (milliseconds % 1000) * 1'000'000L;
    return result;
}
}

namespace net {
TimerFd::TimerFd() : fd_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) {
    // CLOCK_MONOTONIC 适合测量时间间隔，
    // 不受手动修改日历时间导致的时钟跳变影响。
    //
    // TFD_NONBLOCK：没有到期次数时，read 不会一直等待。
    // TFD_CLOEXEC：成功执行 exec 后自动关闭描述符
    if (!fd_.IsValid()) {
        throw std::system_error(errno, std::generic_category(), "timerfd create failed");
    }
}

void TimerFd::Start(int firstDelayMs, int intervalMs) {
    if (firstDelayMs <= 0 || intervalMs < 0) {
        throw std::invalid_argument("first delay must be positive; interval must be nonnegative");
    }

    itimerspec settings{};

    // 第一次在多久之后到期。
    settings.it_value = ToTimespec(firstDelayMs);

    // 第一次之后，每隔多久再次到期。
    // 全零表示不重复。
    settings.it_interval = ToTimespec(intervalMs);

    // flags = 0：it_value 按相对时间解释。
    // 最后一个 nullptr：不需要取回修改前的定时器设置。
    if (::timerfd_settime(fd_.Get(), 0, &settings, nullptr) == -1) {
        throw std::system_error(
            errno,
            std::generic_category(),
            "timerfd_settime failed");
    }
}

void TimerFd::Stop() {
    itimerspec settings{};

    // it_value 全零表示停止定时器。
    // 不是 close，fd 仍然存在。
    if (::timerfd_settime(fd_.Get(), 0, &settings, nullptr) == -1) {
        throw std::system_error(
            errno,
            std::generic_category(),
            "timerfd_settime failed");
    }
}

std::uint64_t TimerFd::Consume() {
    std::uint64_t expirations = 0;

    for (;;) {
        // timerfd 要用 read，读取一个 8 字节的到期计数。
        // read 的返回值是读取字节数，不是到期次数。
        const ssize_t result = ::read(
            fd_.Get(),
            &expirations,
            sizeof(expirations)
        );

        if (result == static_cast<ssize_t>(sizeof(expirations))) {
            return expirations;
        }
        if (result == -1) {
            const int error = errno;

            if (error == EINTR) {
                continue;
            }

            if (error == EAGAIN || error == EWOULDBLOCK) {
                return 0;
            }

            throw std::system_error(
                error,
                std::generic_category(),
                "reading timer failed");
        }

        // 对本例使用的 MONOTONIC timerfd，
        // 成功读取应返回完整计数的字节数。
        throw std::runtime_error("unexpected timer read size");
    }
}

}