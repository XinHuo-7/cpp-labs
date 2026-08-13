cmake_minimum_required(VERSION 3.20)

project(day05_port_lookup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(day05_port_lookup
    main.cpp
    port.cpp
)

target_compile_options(day05_port_lookup PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)