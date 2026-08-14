#include "port_manager.h"

bool PortManager::AddPort(const std::string& name, int speedMbps) {
    if (speedMbps <= 0) {
        return false;
    }

    return ports_.try_emplace(name, name, speedMbps).second;
}

Port* PortManager::FindPort(const std::string& name) {
    auto it = ports_.find(name);
    if (it == ports_.end()) {
        return nullptr;
    }

    return &it->second;
}

bool PortManager::HandlePortEvent(const std::string& name, PortEvent event) {
    Port* port = FindPort(name);
    if (port == nullptr) {
        return false;
    }

    return port->HandleEvent(event);
}

// bool PortManager::SetLinkState(const std::string& name, LinkState state) {
//     Port* port = FindPort(name);

//     if (port == nullptr) {
//         return false;
//     }

//     port->SetLinkState(state);
//     return true;
// }

void PortManager::PrintAll() const {
    for(const auto& item : ports_) {
        item.second.PrintStatus();
    }
}

std::size_t PortManager::GetPortCount() const {
    return ports_.size();
}

