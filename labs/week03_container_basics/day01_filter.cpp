#include "port.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// 创建ports, remove_if + 删除低速端口
// PrintPorts(ports)
void RunFilterDemo() {
    std::vector<Port> ports{
        {"Ethernet0", 10000},
        {"Ethernet4", 25000},
        {"Ethernet8", 100000},
        {"Ethernet12", 1000},
        {"Ethernet16", 40000},
    };

    const int minSpeedMbps{25000};

    std::cout << "删除低于"
              << minSpeedMbps 
              << "Mbps的端口前: \n";
    PrintPorts(ports);

    const auto newEnd = std::remove_if (
        ports.begin(),
        ports.end(),
        [&minSpeedMbps](const Port& port) {
            return port.speedMbps < minSpeedMbps;
        }
    );

    ports.erase(newEnd, ports.end());

    std::cout << "\n 删除后: \n";
    PrintPorts(ports);

}