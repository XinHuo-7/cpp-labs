cmake_minimum_required(VERSION 3.16)
project(week10_modern_cpp LANGUAGES CXX)

add_executable(value_category_demo src/main.cpp)
target_compile_features(value_category_demo PRIVATE cxx_std_17)

enable_testing()
add_test(NAME value_category_test COMMAND value_category_demo)