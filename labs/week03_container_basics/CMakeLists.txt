cmake_minimum_required(VERSION 3.20)

project(week03_container_basics LANGUAGES CXX)

add_executable(week03_container_demos
    main.cpp
    port.cpp
    day01_filter.cpp
    day02_vector_stability.cpp

)

target_compile_features(week03_container_demos PRIVATE cxx_std_20)

target_compile_options(week03_container_demos PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)

if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(week03_container_demos PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    target_link_options(week03_container_demos PRIVATE
        -fsanitize=address,undefined
    )
endif()
