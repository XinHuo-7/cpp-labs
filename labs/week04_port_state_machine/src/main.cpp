#include "port_state_machine.h"

#include <iostream>
#include <exception>
#include <vector>

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
    
    const std::vector<PortEvent> startupEvents{
        {PortEventType::kLinkDetected},
        {PortEventType::kAdminEnable},
        {PortEventType::kLinkDetected},
        {
            PortEventType::kFaultDetected,
            "crc error thresold exceeded"
        },
        {PortEventType::kReset}

    };

    try {
        const std::size_t changedCount = uplinkPort.ApplyBatch(startupEvents);

        std::cout << "本批次实际状态迁移数: " << changedCount << '\n';
        PrintHistory(uplinkPort);
    } catch (const std::exception& error) {
        std::cout << "批量处理失败: " << error.what() << '\n';
    }
    return 0;
}
