#pragma once

#include <string>
#include <vector>

struct Port {
    std::string name;
    int speedMbps;
};

void PrintPorts(const std::vector<Port>& ports);