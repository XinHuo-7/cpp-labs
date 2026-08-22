#include "event_queue.h"

#include <iostream>

void PrintEvent(const PortEvent& event) {
    std::cout << "端口：" << event.portName 
              << " | 目标状态：" << LinkStateToText(event.targetState)
              << "\n";

}

int main() {
    EventQueue queue;

    queue.Push({"Ethernet0", LinkState::kUp});
    queue.Push({"Ethernet4", LinkState::kDown});
    queue.Push({"Ethernet8", LinkState::kUp});

    std::cout << "待处理事件数: " << queue.Size() << "\n\n";

    PortEvent event{"", LinkState::kDown};

    while (queue.TryPop(event))
    {
        PrintEvent(event);
    }
    
    std::cout << "\n队列是否为空: "
              << (queue.Empty() ? "是" : "否")
              << '\n';
    return 0;
}