#include "port.h"

#include <iostream>

Port::Port(const std::string& name, int speedMbps)
    :name_{name}, speedMbps_{speedMbps} {}

const std::string& Port::GetName() const {
    return name_;
}

int Port::GetSpeedMbps() const {
    return speedMbps_;
}

LinkState Port::GetLinkState() const {
    return linkState_;
}

void Port::SetLinkState(LinkState state) {
    linkState_ = state;
}

void Port::PrintStatus() const {
    const char* stateText = 
        linkState_ == LinkState::kUp ? "UP" : "DOWN";

    std::cout << "端口: " << name_
              << " | 状态: " << stateText
              << " | 速率: " << speedMbps_ << "Mbps\n";
}