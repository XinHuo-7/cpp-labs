#include "port.h"

#include <vector>

int main() {
    std::vector<Port> ports;

    ports.emplace_back("Ethernet0", 10000);
    ports.emplace_back("Ethernet4", 25000);
    ports.emplace_back("Ethernet8", 100000);

    ports[0].SetLinkUp(true);
    ports[2].SetLinkUp(true);

    for(const Port& port : ports) {
        port.PrintStatus();
    }

    return 0;
}