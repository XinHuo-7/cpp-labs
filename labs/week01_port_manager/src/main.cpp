#include "port_manager.h"
#include "event_log.h"

#include <iostream>
#include <sstream>
#include <memory>
#include <exception>
#include <utility>

void PrintHelp() {
        std::cout << "可用命令：\n"
              << "  add <name> <speedMbps>\n"
              << "  up <name>\n"
              << "  down <name>\n"
              << "  show <name>\n"
              << "  list\n"
              << "  help\n"
              << "  quit\n";
}

int main(int argc, char* argv[]) {
    const std::string logPath = 
        argc > 1 ? argv[1] : "port_events.log";
    
    try
    {
        auto eventLog = std::make_unique<EventLog>(logPath);
        PortManager manager{std::move(eventLog)};

        std::cout << "Port Manager CLI\n";
        PrintHelp();
        while (true)
        {
            std::cout << "\n> ";
            std::string line;
            std::getline(std::cin, line);

            std::istringstream input(line);
            std::string command;
            input >> command;

            if (command.empty()) {
                continue;
            }

            if (command == "quit") {
                break;
            }
            if (command == "help") {
                PrintHelp();
                continue;
            }

            if (command == "add") {
                std::string name;
                int speedMbps{0};

                input >> name >> speedMbps;

                if (manager.AddPort(name, speedMbps)) {
                    std::cout << "端口添加成功.\n";
                } else {
                    std::cout << "添加失败：端口已存在或速率非法。\n";
                }

                continue;
            }

            if (command == "up" || command == "down") {
                std::string name;
                input >> name;

                const PortEvent event = 
                    command == "up" ? PortEvent::kLinkUp : PortEvent::kLinkDown;
                
                if (manager.HandlePortEvent(name, event)) {
                    std::cout << "端口状态更新成功. \n";
                } else {
                    std::cout << "更新失败：未找到端口或端口状态为变化.\n";
                }

                continue;
            }

            if (command == "show") {
                std::string name;
                input >> name;

                Port* port = manager.FindPort(name);

                if (port != nullptr) {
                    port->PrintStatus();
                } else {
                    std::cout << "未找到端口: " << name << '\n';
                }

                continue;
            }

            if (command == "list") {
                std::cout << "端口总数: " << manager.GetPortCount() << "\n";
                manager.PrintAll();
                continue;
            }

            std::cout << "未知命令, 请输入help查看帮助. \n";
        }

        std::cout << "Port Manager 已退出. \n";    
        return 0;
    }
    catch(const std::exception& error)
    {
        std::cerr << "启动失败：" << error.what() << '\n';
        return 1;
    }
}