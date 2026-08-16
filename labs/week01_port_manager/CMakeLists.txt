cmake_minimum_required(VERSION 3.20)

project(week01_port_manager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(port_core
    src/port.cpp
    src/port_manager.cpp
    src/event_log.cpp
    src/port_policy.cpp
)

target_include_directories(port_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(port_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(port_manager_cli
    src/main.cpp
)

target_link_libraries(port_manager_cli PRIVATE port_core)

include(CTest)

if(BUILD_TESTING)
    add_executable(port_manager_test
        tests/port_manager_test.cpp
    )

    target_link_libraries(port_manager_test PRIVATE port_core)

    add_test(
        NAME port_manager_test
        COMMAND port_manager_test
    )

    add_executable(event_log_test
        tests/event_log_test.cpp
    )

    target_link_libraries(event_log_test PRIVATE port_core)

    add_test(
        NAME event_log_test
        COMMAND event_log_test
    )

    add_executable(port_policy_test
        tests/port_policy_test.cpp
    )

    target_link_libraries(port_policy_test PRIVATE port_core)

    add_test(
        NAME port_policy_test
        COMMAND port_policy_test
    )

endif()