#include "port_state_machine.h"

#include <iostream>

void SendEvent(PortStateMachine& machine, const PortEvent& event) {
    const PortState before = machine.GetState();
    const TransitionResult result = machine.Apply(event);

    std::cout << "端口: " << machine.GetPortName()
              << " | 事件: " << ToText(event.type)
              << " | " << ToText(before)
              << " -> " << ToText(machine.GetState())
              << (result == TransitionResult::kChanged ? "状态已更新" : "事件未生效");
    if(!event.reason.empty()) {
        std::cout << " | 原因:" << event.reason;
    }
    std::cout << '\n';
}
void PrintHistory(const PortStateMachine& machine) {
    const auto& history = machine.GetHistory();

    std::cout << " \n迁移记录数: " << history.size() << '\n';
    for(const auto& record : history) {
        std::cout << "事件: " << ToText(record.event.type)
                  << " | " << ToText(record.before)
                  << " -> " << ToText(record.after)
                  << (record.result == TransitionResult::kChanged ? "状态已更新" : "事件未生效");
        if(!record.event.reason.empty()) {
            std::cout << " | 原因: " << record.event.reason;
        }
        std::cout << '\n';
    }

}

int main() {
    PortStateMachine uplinkPort{"Ethernet0"};

    SendEvent(uplinkPort, {PortEventType::kLinkDetected});  // 状态未变化
    SendEvent(uplinkPort, {PortEventType::kAdminEnable});
    SendEvent(uplinkPort, {PortEventType::kLinkDetected});
    SendEvent(uplinkPort, {PortEventType::kFaultDetected, 
    "remote fault detected"});
    SendEvent(uplinkPort, {PortEventType::kReset});
    SendEvent(uplinkPort, {PortEventType::kLinkLost});  // 状态未变化

    return 0;
}
