cmake_minimum_required(VERSION 3.20)
project(week09_process_ipc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(fork_demo src/fork_demo.cpp)
target_compile_options(fork_demo PRIVATE -Wall -Wextra -Wpedantic)

# Day 2 独立演示，不改动 Day 1 的 fork_demo。
add_executable(exec_demo src/exec_demo.cpp)
target_compile_options(exec_demo PRIVATE -Wall -Wextra -Wpedantic)

# Day 3：独立验证父子进程通过 pipe 传递字节。
add_executable(pipe_demo src/pipe_demo.cpp)
target_compile_options(pipe_demo PRIVATE -Wall -Wextra -Wpedantic)

# Day 4：观察普通变量与共享内存在 fork 后的区别。
add_executable(shared_memory_demo src/shared_memory_demo.cpp)
target_compile_options(shared_memory_demo PRIVATE -Wall -Wextra -Wpedantic)

# Day 5：验证文件映射写入后，可以从文件读回数据。
add_executable(file_mmap_demo src/file_mmap_demo.cpp)
target_compile_options(file_mmap_demo PRIVATE -Wall -Wextra -Wpedantic)

# Day 6：验证 SIGTERM 处理函数只设置标记，退出逻辑留在 main。
add_executable(signal_demo src/signal_demo.cpp)
target_compile_options(signal_demo PRIVATE -Wall -Wextra -Wpedantic)

# Day 7：组合 fork、exec、pipe 和 SIGTERM。
add_executable(process_lifecycle_demo src/process_lifecycle_demo.cpp)
target_compile_options(process_lifecycle_demo PRIVATE -Wall -Wextra -Wpedantic)