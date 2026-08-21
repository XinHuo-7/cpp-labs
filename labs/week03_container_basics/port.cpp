#include "port.h"

#include <string>
#include <vector>
#include <iostream>

void PrintPorts(const std::vector<Port>& ports) {
    for (const Port& port : ports) {
        std::cout << port.name
                  << " | " << port.speedMbps
                  << " Mbps\n ";
    }
}