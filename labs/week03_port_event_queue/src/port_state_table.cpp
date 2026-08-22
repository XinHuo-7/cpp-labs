#include "port_state_table.h"

bool PortStateTable::RegisterPort(const std::string& portName) {
    return states_.try_emplace(portName, LinkState::kDown).second;
}

bool PortStateTable::Apply(const PortEvent& event) {
    auto it = states_.find(event.portName);

    if(it == states_.end()) {
        return false;
    }

    if(it->second == event.targetState) {
        return false;
    }

    it->second = event.targetState;
    return true;
}

bool PortStateTable::TryGetState(const std::string& portName, LinkState& outState) const {
    const auto it = states_.find(portName);

    if(it == states_.end()) {
        return false;
    }
    outState = it->second;
    return true;
}

std::size_t PortStateTable::Size() const {
    return states_.size();
}