cmake_minimum_required(VERSION 3.20)

project(port_state_machine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(port_state_machine
    src/main.cpp
    src/port_state_machine.cpp
)

target_include_directories(port_state_machine PRIVATE include)

target_compile_options(port_state_machine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)