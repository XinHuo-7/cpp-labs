cmake_minimum_required(VERSION 3.16)
project(week11_engineering_v2 LANGUAGES CXX)

find_package(Threads REQUIRED)

add_executable(thread_pool_demo src/thread_pool_demo.cpp)
target_compile_features(thread_pool_demo PRIVATE cxx_std_17)
target_link_libraries(thread_pool_demo PRIVATE Threads::Threads)

enable_testing()
add_test(NAME thread_pool_test COMMAND thread_pool_demo)