#include "event_queue.h"
#include "port_state_table.h"

#include <iostream>

void PrintEvent(const PortEvent& event) {
    std::cout << "端口：" << event.portName 
              << " | 目标状态：" << LinkStateToText(event.targetState)
              << "\n";

}

int main() {
    EventQueue queue;
    PortStateTable stateTable;

    stateTable.RegisterPort("Ethernet0");
    stateTable.RegisterPort("Ethernet4");
    stateTable.RegisterPort("Ethernet8");

    queue.Push({"Ethernet0", LinkState::kUp});
    queue.Push({"Ethernet4", LinkState::kDown});
    queue.Push({"Ethernet8", LinkState::kUp});
    queue.Push({"Ethernet12", LinkState::kUp});

    std::cout << "已处理端口数: " << stateTable.Size() << "\n";
    std::cout << "待处理事件数: " << queue.Size() << "\n\n";

    PortEvent event{"", LinkState::kDown};

    while (queue.TryPop(event))
    {
        PrintEvent(event);
        if (stateTable.Apply(event)) {
            std::cout << " | 状态更新成功\n";
        } else {
            std::cout << " | 状态未生效\n";
        }
    }
    
    std::cout << "\n队列是否为空: "
              << (queue.Empty() ? "是" : "否")
              << '\n';
    return 0;
}