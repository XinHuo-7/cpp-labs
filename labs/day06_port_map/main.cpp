#include "port.h"

#include <iostream>
#include <unordered_map>

int main() {
    std::unordered_map<std::string, Port> ports;
    // ports.emplace("Ethernet0", Port{"Ethernet0", 10000});
    // ports.emplace("Ethernet4", Port{"Ethernet4", 25000});
    // ports.emplace("Ethernet8", Port{"Ethernet8", 100000});
    ports.try_emplace("Ethernet0", "Ethernet0", 10000);
    ports.try_emplace("Ethernet4", "Ethernet4", 25000);
    ports.try_emplace("Ethernet8", "Ethernet8", 100000);
    
    const std::string targeName{"Ethernet8"};

    auto it = ports.find(targeName);
    if (it != ports.end()) {
        it->second.SetLinkUp(true);
        std::cout << "找到目标端口: \n";
        it->second.PrintStatus();
    } else {
        std::cout << "未找到端口: " << targeName << '\n';
    }

    return 0;
}