#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    // fds[0] 是读端，fds[1] 是写端。
    int fds[2]{-1, -1};
    if (::pipe(fds) == -1) {
        std::perror("pipe");
        return 1;
    }

    // 先创建 pipe，再 fork；子进程才能继承这两个描述符。
    const pid_t childPid = ::fork();
    if (childPid == -1) {
        std::perror("fork");
        ::close(fds[0]);
        ::close(fds[1]);
        return 1;
    }

    if (childPid == 0) {
        // 子进程只负责读。必须关闭自己不使用的写端。
        ::close(fds[1]);

        std::string received;
        char buffer[16];
        for (;;) {
            const ssize_t count = ::read(fds[0], buffer, sizeof(buffer));
            if (count > 0) {
                // read 可能只返回部分字节，因此逐次追加。
                received.append(buffer, static_cast<std::size_t>(count));
                continue;
            }
            if (count == 0) {
                // 所有写端都已关闭，且数据读完：到达 EOF。
                break;
            }
            if (errno == EINTR) {
                // 被信号中断时，重新尝试读取。
                continue;
            }
            std::perror("read");
            ::close(fds[0]);
            return 1;
        }
        ::close(fds[0]);
        std::cout << "[child] received=" << received << '\n';
        return received == "PING"? 0 : 1;
    }
    // 父进程只负责写，先关闭自己不使用的读端。
    ::close(fds[0]);
    constexpr char kMessage[]= "PING";
    // sizeof 包含字符串末尾的 '\0'；这里仅发送 P、I、N、G 四个字节。
    const ssize_t written = ::write(fds[1], kMessage, sizeof(kMessage) -1);
    if (written == -1) {
        std::perror("write");
    }
    // 关闭父进程的写端，子进程读完现有数据后才能看到 EOF。
    ::close(fds[1]);

    int status{};
    if (::waitpid(childPid, &status, 0) == -1) {
        std::perror("waitpid");
        return 1;
    }

    if (written != static_cast<ssize_t>(sizeof(kMessage) - 1) ||
        !WIFEXITED(status) || WEXITSTATUS(status) !=0) {
        std::cerr << "pipe demo failed\n";
        return 1;
    }
    std::cout << "[parent] sent=" << written << " child exit=0\n";
    return 0;
}