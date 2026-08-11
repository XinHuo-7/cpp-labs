#include <iostream>
#include <string>

struct PortStatus{
    std::string name;
    bool isUp;
    int speedMbps;
};

std::string LinkStateToText(bool isUp) {
    return isUp ? "UP" : "DOWN";
}

void UpdateSpeed(PortStatus& port, int speedMbps) {
    port.speedMbps = speedMbps;
}

void PrintPortStatus(const PortStatus& port) {
    std::cout << "端口: " << port.name << '\n';
    std::cout << "状态: " << LinkStateToText(port.isUp) << '\n';
    std::cout << "速率: " << port.speedMbps << "Mbps\n";
}

int main() {
    PortStatus uplinkPort{"Ethernet0", false, 10000};
    UpdateSpeed(uplinkPort, 25000);
    PrintPortStatus(uplinkPort);
    return 0;
}
