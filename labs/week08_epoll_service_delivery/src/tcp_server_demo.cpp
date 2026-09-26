#include "tcp_server.h"
#include "logger.h"
#include "server_config.h"
#include "shutdown_signal.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        // 先解析配置，再用配置构造各个业务对象。
        const net::ServerConfig config = net::ParseServerConfig(argc, argv);

        const std::string_view programName = argc > 0 ? std::string_view{argv[0]} : std::string_view{"tcp_server_demo"};

        switch (config.action) {
            case net::ProgramAction::kShowHelp:
                std::cout << net::BuildHelpText(programName);
                return 0;
            
            case net::ProgramAction::kShowVersion:
                std::cout << "tcp_server_demo" << net::kProgramVersion << '\n';
                return 0;
            
            case net::ProgramAction::kRunServer:
                break;
        }

        // Logger 必须先创建，后续启动过程才能使用它记录日志。
        net::Logger logger{
            std::cout,
            config.minimumLogLevel
        };
        // 必须在进入事件循环之前安装信号处理函数。
        net::InstallShutdownSignalHandlers();

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
        
        while (!net::IsShutdownRequested()) {
            // 最多等待 200ms。
            // 有网络、定时器事件时会提前返回。
            server.RunOnce(200);
        }
        logger.Log(
            net::LogLevel::kInfo,
            "shutdown requested; server is stopping"
        );

        // 离开 try 作用域后，server 正常析构：
        // 连接、listener、timerfd 和 epoll fd 都由 RAII 关闭。

    } catch (const std::exception& error) {
        std::cerr << "server failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}