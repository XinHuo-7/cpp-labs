#include "port_manager.h"
#include <utility>
#include <stdexcept>

PortManager::PortManager(
        std::shared_ptr<const PortPolicy> policy,
        std::unique_ptr<EventLog> eventLog)
        : eventlog_(std::move(eventLog)),
          policy_(std::move(policy)) {
        if (policy_ == nullptr) {
            throw std::invalid_argument("PortManager 必须提供 PortPolicy");
        }
}

bool PortManager::AddPort(const std::string& name, int speedMbps) {
    if (!policy_->IsSpeedAllowed(speedMbps)) {
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

