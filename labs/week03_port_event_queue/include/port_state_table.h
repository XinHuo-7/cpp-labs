#pragma once

#include "port_event.h"

#include <cstddef>
#include <string>
#include <unordered_map>

class PortStateTable {
    public:
        bool RegisterPort(const std::string& portName);
        bool Apply(const PortEvent& event);

        bool TryGetState(const std::string& portName,
                         LinkState& outState) const;
        
        std::size_t Size() const;
    private:
        std::unordered_map<std::string, LinkState> states_;
    
};