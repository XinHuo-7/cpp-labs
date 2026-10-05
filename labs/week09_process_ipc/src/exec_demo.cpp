#include <cstdio>
#include <iostream>
#include <string_view>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc,  char* argv[]) {
    // execv 重新加载本程序后，会带着 --child 参数再次进入 main。
    if (argc == 2 && std::string_view{argv[1]} == "--child") {
        std::cout << "[child after exec] pid=" << ::getpid() << " arg=" << argv[1] << '\n';
        return 7;
    }
    if (argc != 1) {
        std::cerr << "Usage: exec_demo\n";
        return 2;
    }

    // fork 前刷新输出，避免父子进程重复输出缓冲区中的文字。
    std::cout << "[start] pid=" << ::getpid() << std::endl;

    const pid_t childPid = ::fork();
    if (childPid == -1) {
        std::perror("fork");
        return 1;
    }
    if (childPid == 0) {
        std::cout << "[child before exec] pid=" << ::getpid() << std::endl;
        // execv 要求参数数组以 nullptr 结尾。
        // nextArgv[0] 是程序名，nextArgv[1] 是新程序收到的参数。
        char childFlag[] = "--child";
        char* const nextArgv[] = {argv[0], childFlag, nullptr};

        // 使用本次运行的路径重新加载程序；成功后不会执行下一行。
        ::execv(argv[0], nextArgv);

        // 只有 execv 失败，才会运行到这里。
        std::perror("execv");
        ::_exit(127);
    }

    int status{};
    if (::waitpid(childPid, &status, 0) == -1) {
        std::perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status)) {
        std::cerr << "child did not exit normally\n";
    }

    std::cout << "[parent] pid=" << ::getpid()
              << " child=" << childPid
              << " exit=" << WEXITSTATUS(status) << '\n';
    return 0;
}