#include "port_state_machine.h"

#include <iostream>

void SendEvent(PortStateMachine& machine, PortEvent event) {
    const PortState before = machine.GetState();
    const TransitionResult result = machine.Apply(event);

    std::cout << "端口: " << machine.GetPortName()
              << " | 事件: " << ToText(event)
              << " | " << ToText(before)
              << " -> " << ToText(machine.GetState())
              << (result == TransitionResult::kChanged ? "状态已更新" : "事件未生效")
              << '\n';

}

int main() {
    PortStateMachine uplinkPort{"Ethernet0"};

    SendEvent(uplinkPort, PortEvent::kLinkDetected);  // 状态未变化
    SendEvent(uplinkPort, PortEvent::kAdminEnable);
    SendEvent(uplinkPort, PortEvent::kLinkDetected);
    SendEvent(uplinkPort, PortEvent::kFaultDetected);
    SendEvent(uplinkPort, PortEvent::kReset);
    SendEvent(uplinkPort, PortEvent::kLinkLost);  // 状态未变化

    return 0;
}
