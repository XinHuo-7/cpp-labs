#include "port_manager.h"
#include <utility>

PortManager::PortManager(std::unique_ptr<EventLog> eventLog)
        : eventlog_(std::move(eventLog)) {
}

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

    const bool changed = port->HandleEvent(event);

    if (changed && eventlog_) {
        eventlog_->Write(name + ": link state changed");
    }

    return changed;
}

void PortManager::PrintAll() const {
    for(const auto& item : ports_) {
        item.second.PrintStatus();
    }
}

std::size_t PortManager::GetPortCount() const {
    return ports_.size();
}

