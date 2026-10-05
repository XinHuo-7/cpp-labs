#include <cstdio>
#include <iostream>

#include <signal.h>

namespace {

// sig_atomic_t 适合在信号处理函数中设置简单标记。
// volatile 表示每次检查都要读取这个值；它不是线程同步工具。

volatile sig_atomic_t gStopRequested = 0;

void HandleTermination(int signalNumber) {
    if (signalNumber == SIGTERM) {
        // 处理函数中只记录请求，不使用 cout，也不做复杂清理。
        gStopRequested = 1;
    }
}

} // namesapce

int main() {
    struct sigaction action{};
    action.sa_handler = HandleTermination;
    action.sa_flags = 0;

    // 明确设置：运行处理函数期间，不额外屏蔽其他信号。
    if (::sigemptyset(&action.sa_mask) == -1) {
        std::perror("sigemptyset");
        return 1;
    }

    // 收到 SIGTERM 时，执行 HandleTermination，而不是默认终止进程。
    if (::sigaction(SIGTERM, &action, nullptr) == -1) {
        std::perror("sigaction");
        return 1;
    }

    std::cout << "[main] running\n";

    // 可重复的测试：向当前程序发送 SIGTERM。
    // 本例中处理函数执行并返回后，raise 才返回。
    if (::raise(SIGTERM) != 0) {
        std::cerr << "raise failed\n";
        return 1;
    }

    if (gStopRequested != 1) {
        std::cerr << "SIGTERM handler did not set the flag\n";
        return 1;
    }

    // 日志和正常退出放在处理函数外执行。
    std::cout << "[main] stop requested\n";
    std::cout << "[main] normal exit\n";
}