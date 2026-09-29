#include <cstdio>
#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    int value = 10;

    // std::endl 会刷新输出缓冲区，避免 fork 后缓冲内容被输出两次。
    std::cout << "[before fork] pid =" << ::getpid() << std::endl;
    
    // fork 调用一次，却会在两个进程中返回：
    // 父进程得到子进程 PID；子进程创建成功返回 0；失败返回 -1。
    const pid_t childPid = ::fork();
    if (childPid == -1) {
        std::perror("fork");
        return 1;
    }
    if (childPid == 0) {
        // 这里仅由子进程执行。修改自己的 value 不会修改父进程的 value。
        value = 20;
        std::cout << "[child] pid=" << ::getpid() << " ppid=" << ::getppid()
                  << " value=" << value << '\n';
        return 7;
    }
    int status{};
    // 父进程等待指定子进程结束，并取得其结束状态。
    if(::waitpid(childPid, &status, 0) == -1) {
        std::perror("waitpid");
        return 1;
    }

    // 只有确认子进程正常退出，才能读取它的退出码。
    if (!WIFEXITED(status)) {
        std::cerr << "child did not exit normally\n";
        return 1;
    }
    std::cout << "[parent] pid=" << ::getpid()
              << " child =" << childPid
              << " value=" << value
              << " child exit=" << WEXITSTATUS(status) << '\n';
    return 0;
}