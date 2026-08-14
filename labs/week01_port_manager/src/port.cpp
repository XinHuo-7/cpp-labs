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

bool Port::HandleEvent(PortEvent event) {
    switch (event)
    {
    case PortEvent::kLinkUp:
        if (linkState_ == LinkState::kUp) {
            return false;
        }
        linkState_ = LinkState::kUp;
        return true;

    case PortEvent::kLinkDown:
        if (linkState_ == LinkState::kDown) {
            return false;
        }
        linkState_ = LinkState::kDown;
        return true;  
   
    }
    
    return false;
}

void Port::PrintStatus() const {
    const char* stateText = 
        linkState_ == LinkState::kUp ? "UP" : "DOWN";

    std::cout << "端口: " << name_
              << " | 状态: " << stateText
              << " | 速率: " << speedMbps_ << "Mbps\n";
}