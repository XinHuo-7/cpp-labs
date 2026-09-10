#include "tcp_socket.h"

#include <exception>
#include <iostream>

int main() {
    try {
        {
            TcpSocket socket;
            std::cout << "TCP socket 创建成功. fd = " << socket.GetFd() << '\n';
            std::cout << "当前尚未监听, 也未建立连接\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "程序失败: " << error.what() << '\n';
        return 1;
    }
    return 0;
}