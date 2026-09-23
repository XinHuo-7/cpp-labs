#include "server_config.h"

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace net {
namespace {

// 读取某个选项后面的值。
// index 使用引用，是因为本函数读取值以后，需要把外层循环的下标向后移动一位，跳过已经使用的参数。

std::string_view ReadOptionValue(int argc, char* argv[], int& index, std::string_view option) {
    if (index + 1 >= argc) {
        throw std::invalid_argument(std::string{"missing value for "} + std::string{option});
    }
    ++index;
    return argv[index];
}

// 将字符串完整转换成 int。
//
// std::from_chars 不抛异常，因此必须同时检查：
// 1. error：转换过程中有没有错误。
// 2. next：是否把整个字符串都读取完了。
int ParseInteger(std::string_view text, std::string_view option) {
    int value{0};
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [next, error] = std::from_chars(begin, end, value);
    if (error != std::errc{} || next != end) {
        throw std::invalid_argument(std::string{option} + "requires a valid integer");
    }
    return value;
}

LogLevel ParseLogLevel(std::string_view text) {
    if (text == "debug") {
        return LogLevel::kDebug;
    }
    if (text == "info") {
        return LogLevel::kInfo;
    }
    if (text == "warning" || text == "warn") {
        return LogLevel::kWarning;
    }
    if (text == "error") {
        return LogLevel::kError;
    }
    throw std::invalid_argument("log level must be debug, info, warning or error");
}
} // namespace

ServerConfig ParseServeConfig(int argc, char*argv[]) {
    ServerConfig config;

    // argv[0] 是程序名称，所以从 argv[1] 开始读取。
    for (int index = 1; index < argc; ++index) {
        const std::string_view option{argv[index]};
        if (option == "--port") {
            const int port = ParseInteger(ReadOptionValue(argc, argv, index, option), option);

            if (port < 0 || port > 65535) {
                throw std::invalid_argument("port must be between 0 and 65535");
            }

            config.port = static_cast<std::uint16_t>(port);
        } else if (option == "--stats-ms") {
            config.statisticsIntervalMs = ParseInteger(ReadOptionValue(argc, argv, index, option), option);

            if (config.statisticsIntervalMs <= 0) {
                throw std::invalid_argument("stats-ms must be greater than 0");
            }

        } else if (option == "--ilde-ms") {
            config.idleTimeoutMs = ParseInteger(ReadOptionValue(argc, argv, index, option), option);
            
            if (config.idleTimeoutMs < 0) {
                throw std::invalid_argument("idle-ms must be not be negative");
            }
        } else if (option == "--log-level") {
            config.minimumLogLevel = ParseLogLevel(ReadOptionValue(argc, argv, index, option));

        } else {
            throw std::invalid_argument(std::string{"unknown option: "} + std::string{option});
        }
    }
    return config;
}
} // namespace net