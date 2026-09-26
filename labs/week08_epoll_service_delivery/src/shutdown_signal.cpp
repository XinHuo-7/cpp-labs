#include "shutdown_signal.h"

#include <cerrno>
#include <csignal>
#include <system_error>

namespace net {
namespace {

// 信号处理函数可能在普通代码执行到任意位置时被调用。

// sig_atomic_t 是适合在普通代码和信号处理函数之间
// 读写的简单整数类型。

// volatile 告诉编译器：这个值可能在当前代码流程之外发生变化，
// 每次检查时都要重新读取。
volatile std::sig_atomic_t stopRequested = 0;

// 信号处理函数必须尽可能简单。
// 这里不能输出日志、分配内存或者操作复杂的 C++ 对象。
void HandleShutdownSignal(int /*signalNumber*/) noexcept {
    stopRequested = 1;
}


void InstallOneHandler(int signalNumber) {
    struct sigaction action{};

    // 收到信号后执行 HandleShutdownSignal。
    action.sa_handler = HandleShutdownSignal;

    // 处理该信号期间不额外阻塞其他信号。
    if(::sigemptyset(&action.sa_mask) == -1) {
        const int errorCode = errno;
        throw std::system_error(errorCode, std::generic_category(), "sigemptyset failed");
    }

    // 不设置 SA_RESTART。
    // 这样 epoll_wait 可以被信号中断并返回 EINTR，
    // 事件循环就有机会重新检查停止标记
    action.sa_flags = 0;

    if (::sigaction(signalNumber, &action, nullptr) == -1) {
        const int errorCode = errno;
        throw std::system_error(errorCode, std::generic_category(), "sigaction failed");
    }


}

} // namespace

void InstallShutdownSignalHandlers() {
    InstallOneHandler(SIGINT);
    InstallOneHandler(SIGTERM);
}

bool IsShutdownRequested() noexcept {
    return stopRequested != 0;
}

} // namespace net