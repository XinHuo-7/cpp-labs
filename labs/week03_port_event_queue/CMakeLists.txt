cmake_minimum_required(VERSION 3.20)

project(week03_port_event_queue LANGUAGES CXX)

add_library(event_core
    src/event_queue.cpp
    src/port_state_table.cpp
)

target_include_directories(event_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(event_core PUBLIC cxx_std_20)

target_compile_options(event_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(port_event_queue_cli
    src/main.cpp
)

target_link_libraries(port_event_queue_cli PRIVATE event_core)


include(CTest)

if(BUILD_TESTING)
    add_executable(event_queue_test
        tests/event_queue_test.cpp
    )

    target_link_libraries(event_queue_test PRIVATE event_core)

    add_test(
        NAME event_queue_test
        COMMAND event_queue_test
    )
    add_executable(port_state_table_test
        tests/port_state_table_test.cpp
    )

    target_link_libraries(port_state_table_test PRIVATE event_core)

    add_test(
        NAME port_state_table_test
        COMMAND port_state_table_test
    )

endif()

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)

if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(SANITIZER_FLAGS
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    target_compile_options(event_core PRIVATE ${SANITIZER_FLAGS})

    function(enable_sanitizers target)
        target_compile_options(${target} PRIVATE
            ${SANITIZER_FLAGS}
        )

        target_link_options(${target} PRIVATE
            -fsanitize=address,undefined
        )
    endfunction()

    enable_sanitizers(port_event_queue_cli)

    if(BUILD_TESTING)
        enable_sanitizers(event_queue_test)
        enable_sanitizers(port_state_table_test)
    endif()
endif()