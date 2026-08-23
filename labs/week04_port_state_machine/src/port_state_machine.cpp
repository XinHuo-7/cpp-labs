#include "port_state_machine.h"

#include <utility>
#include <string>

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

const char* ToText(PortEvent event){
    switch (event)
    {
        case PortEvent::kAdminEnable:
            return "ADMIN_ENABLE";
        case PortEvent::kLinkDetected:
            return "LINK_DETECTED";
        case PortEvent::kLinkLost:
            return "LINK_LOST";
        case PortEvent::kFaultDetected:
            return "FAULT_DETECTED";
        case PortEvent::kReset:
            return "RESET";
    }
    return "UNKNOWN";    
}

PortStateMachine::PortStateMachine(std::string portName)
    :portName_(std::move(portName)) {
}

TransitionResult PortStateMachine::Apply(PortEvent event) {
    const PortState oldState = state_;

    switch (event)
    {
        case PortEvent::kAdminEnable:
            if (state_ == PortState::kDown) {
                state_ = PortState::kNegotiating;
            }
            break;
        
        case PortEvent::kLinkDetected:
            if (state_ == PortState::kNegotiating) {
                state_ = PortState::kUp;
            }
            break;
        
        case PortEvent::kLinkLost:
            if (state_ == PortState::kNegotiating || state_ == PortState::kUp) {
                state_ = PortState::kDown;
            }
            break;
        
        case PortEvent::kFaultDetected:
            if (state_ != PortState::kFault) {
                state_ = PortState::kFault;
            }
            break;
        case PortEvent::kReset:
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