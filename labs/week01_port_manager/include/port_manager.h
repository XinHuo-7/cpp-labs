#pragma once

#include "port.h"

#include <cstddef>
#include<unordered_map>

class PortManager {
public:
    bool AddPort(const std::string& name, int speedMbps);
    Port* FindPort(const std::string& name);

    bool SetLinkState(const std::string& name, LinkState state);
    void PrintAll() const;

    std::size_t GetPortCount() const;
private:
    std::unordered_map<std::string, Port> ports_;
};