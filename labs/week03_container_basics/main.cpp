#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Port {
    std::string name;
    int speedMbps;
};

void PrintPorts(const std::vector<Port>& ports) {
    for (const Port& port : ports) {
        std::cout << port.name
                  << " | " << port.speedMbps
                  << " Mbps\n ";
    }
}

int main() {
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
        [](const Port& port) {
            return port.speedMbps < minSpeedMbps;
        }
    );

    ports.erase(newEnd, ports.end());

    std::cout << "\n 删除后: \n";
    PrintPorts(ports);


    return 0;
}