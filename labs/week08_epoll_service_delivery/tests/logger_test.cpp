#include "logger.h"

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void TestLevelFiltering() {
    std::ostringstream output;

    net::Logger logger{output, net::LogLevel::kWarning};
    logger.Log(net::LogLevel::kDebug, "debug meesage");
    logger.Log(net::LogLevel::kInfo, "info meesage");
    logger.Log(net::LogLevel::kWarning, "idle connection");
    logger.Log(net::LogLevel::kError, "recv failed");

    const std::string expected = "[WARN] idle connection\n"
                                 "[ERROR] recv failed\n";
    Require(
        output.str() == expected,
        "logger did not filter messages correctly"
    );
}

void TestChangingMinimumLevel() {
    std::ostringstream output;
    net::Logger logger{output, net::LogLevel::kError};

    logger.Log(net::LogLevel::kWarning, "first warning");
    // 运行过程中修改最低日志级别。
    logger.SetMinimumLevel(net::LogLevel::kDebug);
    logger.Log(net::LogLevel::kDebug, "debug enabled");
    Require(
        output.str() == "[DEBUG] debug enabled\n",
        "changing minimum log level did not take effect"
    );

}
}

int main() {
    try {
        TestLevelFiltering();
        TestChangingMinimumLevel();
        std::cout << "All logger tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed:" << error.what() << '\n';
        return 1;
    }
}