cmake_minimum_required(VERSION 3.20)

project(concurrent_event_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(event_worker_core
    src/event_worker.cpp
    src/event_queue.cpp
)

target_include_directories(event_worker_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(event_worker_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(event_worker_core PUBLIC
    Threads::Threads
)

add_executable(concurrent_event_service
    src/main.cpp
)

target_link_libraries(concurrent_event_service PRIVATE
    event_worker_core
)

enable_testing()

add_executable(event_queue_test
    tests/event_queue_test.cpp
)

target_link_libraries(event_queue_test PRIVATE event_worker_core)

add_test(
    NAME event_queue_test
    COMMAND event_queue_test
)

add_executable(event_worker_test
    tests/event_worker_test.cpp
)

target_link_libraries(event_worker_test PRIVATE event_worker_core)

add_test(
    NAME event_worker_test
    COMMAND event_worker_test
)