#include "tcp_server.h"

#include <exception>
#include <iostream>

int main() {
    try {
        net::TcpServer server;
        std::cout << "监听127.0.0.1:" << server.GetPort()
                  << "\n按 Ctrl+C 结束演示" << std::endl;
        
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