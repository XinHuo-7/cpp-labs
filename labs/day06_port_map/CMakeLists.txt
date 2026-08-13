cmake_minimum_required(VERSION 3.20)

project(day06_port_map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(day06_port_map
    main.cpp
    port.cpp
)

target_compile_options(day06_port_map PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)