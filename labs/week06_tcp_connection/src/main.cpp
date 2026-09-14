#include "tcp_socket.h"
#include "message_protocol.h"

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
            connection.SetReceiveTimeout(1000);
            client.SetReceiveTimeout(1000);
            std::cout << "[server] state=" << ToText(connection.GetState()) << '\n';

            // day4本次实验约定：请求与响应均为4字节
            // const std::string request = "PING";
            // client.SendAll(request);

            // const std::string receivedRequest = connection.ReceiveExact(4);
            // std::cout << "[server] 收到请求: " << receivedRequest << '\n';

            // const std::string response = "PONG";
            // connection.SendAll(response);

            // const std::string receivedResponse = client.ReceiveExact(4);
            // std::cout << "[client] 收到响应: " << receivedResponse << '\n';

            // day5 SendMessage and ReceiveMessage
            protocol::SendMessage(client, "GET_PORT Ethernet0");
            const std::string request = protocol::ReceiveMessage(connection);
            std::cout << "[server] 收到请求: " << request << '\n';

            protocol::SendMessage(connection, "Ethernet0 UP 10000Mbps");
            const std::string response = protocol::ReceiveMessage(client);
            std::cout << "[client] 收到响应: " << response << '\n';
            connection.Close();
            client.Close();
            std::cout << "[server] state=" << ToText(connection.GetState()) << '\n';
            std::cout << "[client] state=" << ToText(client.GetState()) << '\n';

        }
    } catch (const std::exception& error) {
        std::cerr << "程序失败: " << error.what() << '\n';
        return 1;
    }
    return 0;
}