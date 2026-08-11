cmake_minimum_required(VERSION 3.20)

project(day03_header_source_split LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(day03_header_source_split
    main.cpp
    port.cpp
)

target_compile_options(day03_header_source_split PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)