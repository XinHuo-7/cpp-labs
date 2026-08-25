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
