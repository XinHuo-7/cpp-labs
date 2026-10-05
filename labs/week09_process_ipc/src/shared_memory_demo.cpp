#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    int localValue = 0;
    constexpr std::size_t kSize = sizeof(int);

    // MAP_SHARED：修改对共享这块映射的父子进程可见。
    // MAP_ANONYMOUS：不映射文件，所以 fd 传 -1、offset 传 0。
    // 必须在 fork 前创建，子进程才会继承这块映射。
    void* const memory = ::mmap(
        nullptr,
        kSize,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    // mmap 失败返回 MAP_FAILED，不是 nullptr。
    if (memory == MAP_FAILED) {
        std::perror("mmap");
        return 1;
    }

    const pid_t childPid = ::fork();
    if (childPid == -1) {
        std::perror("fork");
        ::munmap(memory, kSize);
        return 1;
    }

    if (childPid == 0) {
        localValue = 42; // 只修改子进程自己的普通变量副本。
        constexpr int KNewValue = 42;
        // 把整数的字节写进共享映射；父进程稍后可从同一映射读出。
        std::memcpy(memory, &KNewValue, sizeof(KNewValue));

        // 子进程完成写入后退出；_exit 不会返回父进程的执行路线。
        ::_exit(0);
    }

    int status{};
    if (::waitpid(childPid, &status, 0) == -1) {
        std::perror("waitpid");
        ::munmap(memory, kSize);
        return 1;
    }

    // 先等子进程结束，再读共享内存：本例不发生同时读写。
    int sharedValue{};
    std::memcpy(&sharedValue, memory, sizeof(sharedValue));

    if (::munmap(memory, kSize) == -1) {
        std::perror("munmap");
        return 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 
        || localValue != 0 || sharedValue != 42) {
            std::cerr << "shared memory demo failed";
            return 1;
    }
    std::cout << "[parent] local=" << localValue
              << " shared=" << sharedValue << '\n';
    return 0;

}