#include "tcp_server.h"
#include "logger.h"
#include "server_config.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        // 先解析配置，再用配置构造各个业务对象。
        const net::ServerConfig config = net::ParseServerConfig(argc, argv);

        // Logger 必须先创建，后续启动过程才能使用它记录日志。
        net::Logger logger{
            std::cout,
            config.minimumLogLevel
        };

        // TcpServer 构造期间会创建监听 socket、绑定端口并加入 epoll。
        net::TcpServer server{
            config.port,
            config.statisticsIntervalMs,
            config.idleTimeoutMs,

            // &logger 取得 Logger 对象的地址。
            // TcpServer 不拥有该对象，只在运行期间使用它。
            &logger
        };
        // std::cout << "监听127.0.0.1:" << server.GetPort()
        //           << "\n按 Ctrl+C 结束演示" << std::endl;
        const std::string listenMessage =
            "监听 127.0.0.1:" +
            std::to_string(server.GetPort());
        logger.Log(net::LogLevel::kInfo, listenMessage);
        logger.Log(net::LogLevel::kInfo,"按 Ctrl+C 结束演示");
        for (;;) {
            // 没有事件时允许等待，避免空转消耗 CPU。
            // socket 非阻塞，不代表 epoll_wait 也必须不等待。

            // 没有事件就等待；定时器到期同样会唤醒 epoll_wait。
            // -1 不代表停止处理，也不代表定时器不会触发。
            server.RunOnce(-1);
        }
    } catch (const std::exception& error) {
        std::cerr << "server failed: " << error.what() << '\n';
        return 1;
    }
}