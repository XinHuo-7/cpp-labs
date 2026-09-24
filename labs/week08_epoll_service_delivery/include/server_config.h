#pragma once

#include "logger.h"

#include <cstdint>

namespace net {

// 集中保存服务端启动配置。
// 使用默认成员初始化，即使用户不传参数也能直接启动。
struct ServerConfig {
    // 端口为 0 时，由操作系统自动分配可用端口。
    std::uint16_t port{0};

    // 定时统计和空闲连接检查周期。
    int statisticsIntervalMs{1000};

    // 连接超过该时间没有活动就关闭；0 表示禁用空闲清理。
    int idleTimeoutMs{5000};

    // 默认过滤 DEBUG 日志。
    LogLevel minimumLogLevel{LogLevel::kInfo};
};

// argc、argv 直接接收 main() 传入的命令行参数。
// 解析失败时抛出 std::invalid_argument。
ServerConfig ParseServerConfig(int argc, char* argv[]);

} // namespace net