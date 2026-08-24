cmake_minimum_required(VERSION 3.20)

project(port_state_machine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(port_state_machine_core
    src/port_state_machine.cpp
)

target_include_directories(port_state_machine_core PUBLIC include)

target_compile_options(port_state_machine_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(port_state_machine
    src/main.cpp
)

target_link_libraries(port_state_machine PRIVATE
    port_state_machine_core
)

include(CTest)

if(BUILD_TESTING)
    add_executable(port_state_machine_test
        tests/port_state_machine_test.cpp
    )

    target_link_libraries(port_state_machine_test PRIVATE
        port_state_machine_core
    )

    add_test(
        NAME port_state_machine_test
        COMMAND port_state_machine_test
    )
endif()