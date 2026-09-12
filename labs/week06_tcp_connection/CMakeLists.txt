cmake_minimum_required(VERSION 3.20)

project(tcp_connection LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

add_library(tcp_connection_core
    src/tcp_socket.cpp
    src/message_protocol.cpp
)

target_include_directories(tcp_connection_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(tcp_connection_demo
    src/main.cpp
)

target_link_libraries(tcp_connection_demo PRIVATE
    tcp_connection_core
)

include(CTest)

if(BUILD_TESTING)
    add_executable(tcp_socket_test
        tests/tcp_socket_test.cpp
    )

    target_link_libraries(tcp_socket_test PRIVATE
        tcp_connection_core
    )

    add_test(
        NAME tcp_socket_test
        COMMAND tcp_socket_test
    )

    set_tests_properties(tcp_socket_test PROPERTIES TIMEOUT 10)

    add_executable(message_protocol_test
        tests/message_protocol_test.cpp
    )

    target_link_libraries(message_protocol_test PRIVATE
        tcp_connection_core
    )

    add_test(
        NAME message_protocol_test
        COMMAND message_protocol_test
    )

    set_tests_properties(message_protocol_test PROPERTIES
        TIMEOUT 10
)
endif()