cmake_minimum_required(VERSION 3.16)
project(week10_modern_cpp LANGUAGES CXX)

add_executable(value_category_demo src/main.cpp)
target_compile_features(value_category_demo PRIVATE cxx_std_17)

enable_testing()
add_test(NAME value_category_test COMMAND value_category_demo)

add_executable(forward_demo src/day2_forward_demo.cpp)
target_compile_features(forward_demo PRIVATE cxx_std_17)
add_test(NAME forward_test COMMAND forward_demo)

add_executable(template_demo src/day3_template_demo.cpp)
target_compile_features(template_demo PRIVATE cxx_std_17)
add_test(NAME template_test COMMAND template_demo)

add_executable(constexpr_demo src/day4_constexpr_demo.cpp)
target_compile_features(constexpr_demo PRIVATE cxx_std_17)
add_test(NAME constexpr_test COMMAND constexpr_demo)

add_executable(optional_demo src/day5_optional_demo.cpp)
target_compile_features(optional_demo PRIVATE cxx_std_17)
add_test(NAME optional_test COMMAND optional_demo)

add_executable(variant_demo src/day6_variant_demo.cpp)
target_compile_features(variant_demo PRIVATE cxx_std_17)
add_test(NAME variant_test COMMAND variant_demo)