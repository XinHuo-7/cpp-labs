#include "shutdown_signal.h"

#include <csignal>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void TestShutdownSignalSetsFlag() {
    net::InstallShutdownSignalHandlers();
    Require(!net::IsShutdownRequested(), "shutdown was requested before receiving a signal");

    // raise() 向当前测试进程发送 SIGINT。
    // 由于已经安装处理函数，进程不会被直接终止。
    if (std::raise(SIGINT) != 0) {
        throw std::runtime_error("raise SIGINT failed");
    }
    Require(net::IsShutdownRequested(), "SIGINT did not set shutdown flag");
}

} //namesapce

int main () {
    try {
        TestShutdownSignalSetsFlag();
        std::cout << "All shutdown signal tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed" << error.what() << '\n';
        return 1;
    }
}