#include "server_config.h"

#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void TestParseCustomConfig() {
    // argv 要求 char*，所以这里使用可修改的字符数组。
    char program[] = "tcp_server_demo";
    char portOption[] = "--port";
    char portValue[] = "8080";
    char statsOption[] = "--stats-ms";
    char statsValue[] = "2000";
    char idleOption[] = "--idle-ms";
    char idleValue[] = "10000";
    char logOption[] = "--log-level";
    char logValue[] = "debug";
    std::array<char*, 9> arguments {
        program,
        portOption,
        portValue,
        statsOption,
        statsValue,
        idleOption,
        idleValue,
        logOption,
        logValue
    };
    const net::ServerConfig config = net::ParseServerConfig(static_cast<int>(arguments.size()), arguments.data());
    Require(config.port == 8080, "port was not parsed");
    Require(config.statisticsIntervalMs == 2000, "statistcs interval was not parsed");
    Require(config.idleTimeoutMs == 10000, "idle timeout was not parsed");
    Require(config.minimumLogLevel == net::LogLevel::kDebug, "log level was not parsed");
}

void TestRejectInvalidPort() {
    char program[] = "tcp_server_demo";
    char option[] = "--port";
    char value[] = "70000";

    char* arguments[]{program, option, value};
    bool rejected{false};
    try {
        (void)net::ParseServerConfig(3, arguments);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Require(rejected, "invalid port was not rejected");
} 
} // namespace

int main() {
    try {
        TestParseCustomConfig();
        TestRejectInvalidPort();

        std::cout << "All server config tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: "
                  << error.what() << '\n';
        return 1;
    }
}