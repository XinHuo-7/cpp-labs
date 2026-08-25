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

    void ValidateEvent(const PortEvent& event) {
        if (event.type == PortEventType::kFaultDetected && event.reason.empty()) {
            throw std::invalid_argument("fault event requires a reason");
        }
    }

    PortState CalculateNextState(PortState current, PortEventType eventType) {

        switch (eventType)
        {
            case PortEventType::kAdminEnable:
                return current == PortState::kDown ?
                    PortState::kNegotiating : current;
            
            case PortEventType::kLinkDetected:
                return current == PortState::kNegotiating ?
                    PortState::kUp : current;
            
            case PortEventType::kLinkLost:
                return current == PortState::kNegotiating || current == PortState::kDown ?
                    PortState::kDown : current;
            
            case PortEventType::kFaultDetected:
                return current != PortState::kFault
                    ? PortState::kFault
                    : current;

            case PortEventType::kReset:
                return current == PortState::kFault
                    ? PortState::kDown
                    : current;  
        }
        return current;
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
    ValidateEvent(event);

    const PortState nextState = CalculateNextState(state_, event.type);
    

    if (nextState == state_) {
        return TransitionResult::kIgnored;
    }

    state_ = nextState;
    return TransitionResult::kChanged;

}

PortState PortStateMachine::GetState() const {
    return state_;
}

const std::string& PortStateMachine::GetPortName() const {
    return portName_;
}