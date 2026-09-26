# CMake generated Testfile for 
# Source directory: /home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery
# Build directory: /home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[io_utils_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/io_utils_test")
set_tests_properties([=[io_utils_test]=] PROPERTIES  TIMEOUT "10" _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;89;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
add_test([=[epoll_poller_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/epoll_poller_test")
set_tests_properties([=[epoll_poller_test]=] PROPERTIES  TIMEOUT "10" _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;106;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
add_test([=[epoll_et_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/epoll_et_test")
set_tests_properties([=[epoll_et_test]=] PROPERTIES  TIMEOUT "10" _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;124;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
add_test([=[tcp_server_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/tcp_server_test")
set_tests_properties([=[tcp_server_test]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;141;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
add_test([=[timer_fd_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/timer_fd_test")
set_tests_properties([=[timer_fd_test]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;158;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
add_test([=[core_debug_normal_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/core_debug_demo")
set_tests_properties([=[core_debug_normal_test]=] PROPERTIES  PASS_REGULAR_EXPRESSION "Normal path passed" TIMEOUT "5" _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;169;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
add_test([=[logger_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/logger_test")
set_tests_properties([=[logger_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;187;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
add_test([=[server_config_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/server_config_test")
set_tests_properties([=[server_config_test]=] PROPERTIES  TIMEOUT "5" _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;200;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
add_test([=[shutdown_signal_test]=] "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/build-debug/shutdown_signal_test")
set_tests_properties([=[shutdown_signal_test]=] PROPERTIES  TIMEOUT "5" _BACKTRACE_TRIPLES "/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;217;add_test;/home/xinhuo/code/cpp-labs/labs/week08_epoll_service_delivery/CMakeLists.txt;0;")
