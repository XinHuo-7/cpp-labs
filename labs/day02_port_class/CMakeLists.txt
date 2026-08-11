cmake_minimum_required(VERSION 3.20)

project(day02_port_class LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(day02_port_class main.cpp)

target_compile_options(day02_port_class PRIVATE
    -Wall
    -Wextra 
    -Wpedantic
)