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
endif()