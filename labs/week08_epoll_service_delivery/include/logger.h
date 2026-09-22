#pragma once

#include <ostream>
#include <string_view>

namespace net {

// 日志级别按照严重程度从低到高排列。
// 后面可以直接比较枚举对应的整数值，实现日志过滤。
enum class LogLevel {
    kDebug = 0,
    kInfo = 1,
    kWarning = 2,
    kError = 3
};

// 返回的 string_view 指向字符串常量，不负责管理字符串内存。
std::string_view ToString(LogLevel level) noexcept;

class Logger {
public:
    // output 使用引用：
    // Logger 不拥有 cout、cerr 或文件流，只借用外部输出流。
    explicit Logger(std::ostream& output, LogLevel minimumLevel = LogLevel::kInfo) noexcept;

    void SetMinimumLevel(LogLevel level) noexcept;
    LogLevel GetMinimumLevel() const noexcept;
    bool ShouldLog(LogLevel level) const noexcept;
    void Log(LogLevel LogLevel, std::string_view message);

private:
    std::ostream& output_;
    LogLevel minimumLevel_;
};


    
} // namespace net
