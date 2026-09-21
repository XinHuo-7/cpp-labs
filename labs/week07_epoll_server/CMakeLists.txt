cmake_minimum_required(VERSION 3.20)

project(week07_epoll_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

add_library(epoll_core
    src/io_utils.cpp
    src/epoll_poller.cpp
    src/tcp_server.cpp
    src/timer_fd.cpp
)

# 为 Linux 的 accept4 等扩展接口提供声明。
target_compile_definitions(epoll_core PRIVATE
    _GNU_SOURCE
)
add_executable(tcp_server_demo
    src/tcp_server_demo.cpp
)

target_link_libraries(tcp_server_demo PRIVATE
    epoll_core
)

target_include_directories(epoll_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(nonblocking_io_demo
    src/main.cpp
)
add_executable(epoll_demo
    src/epoll_demo.cpp
)
target_link_libraries(nonblocking_io_demo PRIVATE
    epoll_core
)
target_link_libraries(epoll_demo PRIVATE
    epoll_core
)

add_executable(core_debug_demo
    src/core_debug_demo.cpp
)

# 仅用于排障练习目标：
# -g 生成调试信息；-O0 关闭优化，方便观察变量和调用栈。
target_compile_options(core_debug_demo PRIVATE
    -g
    -O0
)

include(CTest)

if(BUILD_TESTING)
    add_executable(io_utils_test
        tests/io_utils_test.cpp
    )

    target_link_libraries(io_utils_test PRIVATE
        epoll_core
    )

    add_test(
        NAME io_utils_test
        COMMAND io_utils_test
    )

    set_tests_properties(io_utils_test PROPERTIES
        TIMEOUT 10
    )
    
    add_executable(epoll_poller_test
    tests/epoll_poller_test.cpp
    )

    target_link_libraries(epoll_poller_test PRIVATE
        epoll_core
    )

    add_test(
        NAME epoll_poller_test
        COMMAND epoll_poller_test
    )

    # 这是测试进程的兜底超时，不是 socket 或 epoll 的配置。
    set_tests_properties(epoll_poller_test PROPERTIES
        TIMEOUT 10
    )

    add_executable(epoll_et_test
    tests/epoll_et_test.cpp
    )

    target_link_libraries(epoll_et_test PRIVATE
        epoll_core
    )

    add_test(
        NAME epoll_et_test
        COMMAND epoll_et_test
    )

    # 即使实现出错，也不允许测试无限等待。
    set_tests_properties(epoll_et_test PROPERTIES
        TIMEOUT 10
    )
    add_executable(tcp_server_test
        tests/tcp_server_test.cpp
    )

    target_link_libraries(tcp_server_test PRIVATE
        epoll_core
    )

    add_test(
        NAME tcp_server_test
        COMMAND tcp_server_test
    )

    # 即使网络操作或事件循环实现有误，也不能无限挂住测试。
    set_tests_properties(tcp_server_test PROPERTIES
        TIMEOUT 15
    )
    add_executable(timer_fd_test
        tests/timer_fd_test.cpp
    )

    target_link_libraries(timer_fd_test PRIVATE
        epoll_core
    )

    add_test(
        NAME timer_fd_test
        COMMAND timer_fd_test
    )

    # 测试进程的兜底超时，不是定时器的触发周期。
    set_tests_properties(timer_fd_test PROPERTIES
        TIMEOUT 15
    )

    # 自动化测试只执行正常模式，不传 --crash。
    add_test(
        NAME core_debug_normal_test
        COMMAND core_debug_demo
    )

    set_tests_properties(core_debug_normal_test PROPERTIES
        TIMEOUT 5
        PASS_REGULAR_EXPRESSION "Normal path passed"
    )
endif()