cmake_minimum_required(VERSION 3.20)

project(day04_port_vector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(day04_port_vector
    main.cpp
    port.cpp
)

target_compile_options(day04_port_vector PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)