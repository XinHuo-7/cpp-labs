#include "port.h"

#include <algorithm>
#include <vector>
#include <iostream>

int main() {
    std::vector<Port> ports;
    ports.emplace_back("Ethernet0", 10000);
    ports.emplace_back("Ethernet4", 25000);
    ports.emplace_back("Ethernet8", 100000);

    const std::string targetName("Ethernet12");

    auto it = std::find_if(
        ports.begin(),
        ports.end(),
        [&targetName](const Port& port) {
            return port.GetName() == targetName;
        }
    );
    if (it != ports.end()) {
        it->SetLinkUp(true);
        std::cout << "找到目标端口: \n";
        it->PrintStatus();
    } else {
        std::cout << "未找到端口: " << targetName << '\n';
    }
}