#include "port_state_machine.h"

#include <utility>
#include <string>
#include <stdexcept>

namespace {

    std::string ValidatePortName(std::string portName) {
        if (portName.empty()) {
            throw std::invalid_argument("port name cannot be empty");
        }
        return portName;
    }
}

const char* ToText(PortState state) {
    switch (state)
    {
        case PortState::kDown:
            return "DOWN";
        case PortState::kNegotiating:
            return "NEGOTIATING";
        case PortState::kUp:
            return "UP";
        case PortState::kFault:
            return "FAULT";
    }
    return "UNKNOWN";
}

const char* ToText(PortEventType type){
    switch (type)
    {
        case PortEventType::kAdminEnable:
            return "ADMIN_ENABLE";
        case PortEventType::kLinkDetected:
            return "LINK_DETECTED";
        case PortEventType::kLinkLost:
            return "LINK_LOST";
        case PortEventType::kFaultDetected:
            return "FAULT_DETECTED";
        case PortEventType::kReset:
            return "RESET";
    }
    return "UNKNOWN";    
}

PortStateMachine::PortStateMachine(std::string portName)
    : portName_(ValidatePortName(std::move(portName))) {
}

TransitionResult PortStateMachine::Apply(const PortEvent& event) {
    if (event.type == PortEventType::kFaultDetected &&
        event.reason.empty()) {
        throw std::invalid_argument("fault event requires a reason");

    }

    
    const PortState oldState = state_;

    switch (event.type)
    {
        case PortEventType::kAdminEnable:
            if (state_ == PortState::kDown) {
                state_ = PortState::kNegotiating;
            }
            break;
        
        case PortEventType::kLinkDetected:
            if (state_ == PortState::kNegotiating) {
                state_ = PortState::kUp;
            }
            break;
        
        case PortEventType::kLinkLost:
            if (state_ == PortState::kNegotiating || state_ == PortState::kUp) {
                state_ = PortState::kDown;
            }
            break;
        
        case PortEventType::kFaultDetected:
            if (state_ != PortState::kFault) {
                state_ = PortState::kFault;
            }
            break;
        case PortEventType::kReset:
            if (state_ == PortState::kFault) {
                state_ = PortState::kDown;
            }
            break;  
    }
    return state_ == oldState ? TransitionResult::kIgnored : TransitionResult::kChanged;
}

PortState PortStateMachine::GetState() const {
    return state_;
}

const std::string& PortStateMachine::GetPortName() const {
    return portName_;
}