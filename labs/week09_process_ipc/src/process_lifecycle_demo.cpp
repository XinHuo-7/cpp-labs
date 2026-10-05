#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

int RunWorker() {
    // exec 后重新进入 main，带 --worker 参数时走到这里。
    // 子进程的标准输入已经被接到 pipe 的读端。
    std::string message;
    char buffer[16];
    for (;;) {
        const ssize_t count = ::read(STDIN_FILENO, buffer, sizeof(buffer));
        if (count > 0) {
            message.append(buffer, static_cast<std::size_t>(count));
            continue;
        }
        if (count == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        std::perror("worker read");
        return 1;
    }

    sigset_t termSet{};
    if (::sigemptyset(& termSet) == -1 || ::sigaddset(&termSet, SIGTERM) == -1) {
        std::perror("worker signal set");
        return 1;
    }

    // SIGTERM 在 fork 前就已被屏蔽，exec 后仍保持屏蔽。
    // sigwait 同步接收它；这里不在异步信号处理函数中做清理。
    int receivedSignal = 0;
    const int error = ::sigwait(&termSet, &receivedSignal);
    if (error != 0) {
        std::cerr << "sigwait failed, error=" << error << '\n';
        return 1;
    }

    if (message != "PING" || receivedSignal != SIGTERM) {
        std::cerr << "worker received unexpected data or signal\n";
        return 1;
    }
    std::cout << "[worker] received=PING, SIGTERM received, exit=0\n";
    return 0;
}
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view{argv[1]} == "--worker") {
            if (::isatty(STDIN_FILENO) == 1) {
            std::cerr << "--worker 是内部模式，请直接运行程序\n";
            return 2;
        }
        return RunWorker();
    }

    sigset_t termSet{};
    if (::sigemptyset(&termSet) == -1 || ::sigaddset(&termSet, SIGTERM) == -1) {
        std::perror("signal set");
        return 1;
    }

    // 在 fork 前屏蔽 SIGTERM：子进程会继承该设置。
    // 即使父进程很快发送信号，子进程也不会在准备阶段被直接终止。
    if (::sigprocmask(SIG_BLOCK, &termSet, nullptr) == -1) {
        std::perror("sigpromask");
        return 1;
    }

    int fds[2] {-1, -1};
    if (::pipe(fds) == -1) {
        std::perror("pipe");
        ::close(fds[0]);
        ::close(fds[1]);
        return 1;
    }

    const pid_t childPid = ::fork();
    if (childPid == -1) {
        std::perror("fork");
        ::close(fds[0]);
        ::close(fds[1]);
        return 1;
    }
    if (childPid == 0) {
        ::close(fds[1]); // 子进程只读。

        // 将 pipe 读端接到标准输入 fd=0。
        // exec 后新程序就能用 read(STDIN_FILENO, ...) 读取它。
        if (fds[0] != STDIN_FILENO) {
            if(::dup2(fds[0], STDIN_FILENO) == -1) {
                std::perror("dup2");
                ::_exit(127);
            }
            ::close(fds[0]);
        }

        char workerFlag[] = "--worker";
        char* const nextArgv[] = {argv[0], workerFlag, nullptr};

        // 成功后不会返回；同一个子进程重新从 main 开始运行。
        ::execv(argv[0], nextArgv);
        std::perror("execv");
        ::_exit(127);
    }

    ::close(fds[0]); // 父进程只写。

    constexpr char kMessage[] = "PING";
    const ssize_t written = ::write(fds[1], kMessage, sizeof(kMessage) - 1);
    if (written == -1) {
        std::perror("parent write");
    }
    ::close(fds[1]); // 告诉子进程：不会再有数据，读端最终可得到 EOF。

    if (::kill(childPid, SIGTERM) == -1) {
        std::perror("kill");
        ::kill(childPid, SIGKILL); // 演示失败时，避免留下等待信号的子进程。
        ::waitpid(childPid, nullptr, 0);
        return 1;
    }

    int status{};
    if (::waitpid(childPid, &status, 0) == -1) {
        std::perror("waitpid");
        return 1;
    }

    if (written != static_cast<ssize_t>(sizeof(kMessage) - 1) || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::cerr << "process lifecycle demo failed\n";
        return 1;
    }

    std::cout << "[parent] child exit=0\n";
    return 0;

}