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
                return current == PortState::kNegotiating || current == PortState::kUp ?
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

    StateTransition BuildTransition(PortState before, const PortEvent& event) {
        ValidateEvent(event);

        const PortState after = CalculateNextState(before, event.type);

        const TransitionResult result = after == before ? TransitionResult::kIgnored
            : TransitionResult::kChanged;
        
        return {
            before,
            after,
            event,
            result,
        };

        
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
    const StateTransition transition = BuildTransition(state_, event);
    history_.push_back(transition);
    state_ = transition.after;
    return transition.result;

}

std::size_t PortStateMachine::ApplyBatch(const std::vector<PortEvent>& events) {
    PortState stagedState = state_;
    std::vector<StateTransition> stagedRecords;
    stagedRecords.reserve(events.size());

    std::size_t changedCount = 0;
    // 阶段一: 只在临时变量中模拟，不修改对象本身
    for(const PortEvent& event : events) {
        StateTransition transition = BuildTransition(stagedState, event);
        if (transition.result == TransitionResult::kChanged) {
            changedCount++;
        }

        stagedState = transition.after;
        stagedRecords.push_back(std::move(transition));
    }
    // 阶段二：复制原历史，并在副本上追加新纪录
    // 任意复制、扩容异常都不会影响history_, state_.
    std::vector<StateTransition> nextHistory = history_;
    nextHistory.reserve(history_.size() + stagedRecords.size());
    for (const StateTransition& record : stagedRecords) {
        nextHistory.push_back(record);
    }

    // 两步提交：交换历史，再提交枚举状态
    history_.swap(nextHistory);
    state_ = stagedState;

    return changedCount;
}

PortState PortStateMachine::GetState() const {
    return state_;
}

const std::string& PortStateMachine::GetPortName() const {
    return portName_;
}

const std::vector<StateTransition>& PortStateMachine::GetHistory() const {
    return history_;
}