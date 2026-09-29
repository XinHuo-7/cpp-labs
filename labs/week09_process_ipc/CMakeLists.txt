cmake_minimum_required(VERSION 3.20)
project(week09_process_ipc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(fork_demo src/fork_demo.cpp)
target_compile_options(fork_demo PRIVATE -Wall -Wextra -Wpedantic)