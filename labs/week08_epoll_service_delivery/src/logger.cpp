#include "logger.h"

namespace net {

std::string_view ToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::kDebug:
            return "DEBUG";
        case LogLevel::kInfo:
            return "INFO";
        case LogLevel::kWarning:
            return "WARN";
        case LogLevel::kError:
            return "ERROR";
    }
    return "UNKNOWN";
}

Logger::Logger(std::ostream& output, LogLevel minimumLevel) noexcept
        : output_(output), minimumLevel_(minimumLevel) {}

void Logger::SetMinimumLevel(LogLevel level) noexcept {
    minimumLevel_ = level;
}

LogLevel Logger::GetMinimumLevel() const noexcept {
    return minimumLevel_;
}

bool Logger::ShouldLog(LogLevel level) const noexcept {
    // enum class 不能直接和整数比较，
    // 因此显式转换为 int 后比较严重程度。
    return static_cast<int>(level) >= static_cast<int>(minimumLevel_);
}

void Logger::Log(LogLevel level, std::string_view message) {
    if(!ShouldLog(level)) {return;}
    output_ << '[' << ToString(level) << "] " << message << '\n';
}

}