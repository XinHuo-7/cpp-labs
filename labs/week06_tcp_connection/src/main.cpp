#include "tcp_socket.h"

#include <exception>
#include <iostream>
#include <string>

int main() {
    try {
        {
            TcpSocket listener;
            listener.BindLoopback(0);
            listener.Listen();

            const auto serverPort = listener.GetLocalPort();

            std::cout << "服务器监听 127.0.0.1:" << listener.GetLocalPort() << '\n';

            TcpSocket client;
            client.ConnectLoopback(serverPort);
            std::cout << "客户端连接成功，本地端口:" << client.GetLocalPort() << '\n';
           
            TcpSocket connection = listener.Accept();
            std::cout << "服务器已接受连接\n";
            std::cout << "监听 Socket fd: " << listener.GetFd() << '\n';
            std::cout << "客户端 Socket fd: " << client.GetFd() << '\n';
            std::cout << "服务端连接Socket fd: " << connection.GetFd() << '\n';
            std::cout << "按回车退出并自动关闭三个 Socket \n";

            std::string input;
            std::getline(std::cin, input);
        }
    } catch (const std::exception& error) {
        std::cerr << "程序失败: " << error.what() << '\n';
        return 1;
    }
    return 0;
}