#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    constexpr char KMessage[] = "PING";
    constexpr std::size_t kBytes = sizeof(KMessage) - 1;
    constexpr const char* kPath = "build/day5_mmap.bin";

    // 打开或创建文件。O_TRUNC 让每次演示都从空文件开始。
    const int fd = ::open(kPath, O_TRUNC | O_CREAT | O_RDWR, 0600);
    if (fd == -1) {
        std::perror("open");
        return 1;
    }

    // 新文件长度为 0；映射并写入前，先把它扩展到 4 字节。
    if (::ftruncate(fd, static_cast<off_t>(kBytes)) == -1) {
        std::perror("ftruncate");
        ::close(fd);
        return 1;
    }

    // 与 Day 4 不同：这里 fd 指向真实文件，没有 MAP_ANONYMOUS。
    // MAP_SHARED 使对映射的修改能够写回对应文件。
    void* const memory = ::mmap(
        nullptr,
        kBytes,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );
    if (memory == MAP_FAILED) {
        std::perror("mmap");
        ::close(fd);
        return 1;
    }

    // 写的是内存地址，不是调用 write(fd, ...)。
    std::memcpy(memory, KMessage, kBytes);

    // MS_SYNC：等待映射中的修改同步到文件。
    if (::msync(memory, kBytes, MS_SYNC) == -1) {
        std::perror("msync");
        ::munmap(memory, kBytes);
        ::close(fd);
        return 1;
    }

    if (::munmap(memory, kBytes) == -1) {
        std::perror("munmap");
        ::close(fd);
        return 1;
    }

    char buffer[kBytes]{};
    // 从文件偏移量 0 读回；pread 不改变 fd 当前的读写位置。
    const ssize_t count = ::pread(fd, buffer, kBytes, 0);
    if (count == -1) {
        std::perror("pread");
        ::close(fd);
        return 1;
    }

    if (::close(fd) == -1) {
        std::perror("close");
        return 1;
    }

    if (count != static_cast<ssize_t>(kBytes) || std::memcmp(buffer, KMessage, kBytes) != 0) {
        std::cerr << "file map verification failed\n";
        return 1;
    }

    std::cout << "[file map] read=";
    std::cout.write(buffer, kBytes) << '\n';
    return 0;
}