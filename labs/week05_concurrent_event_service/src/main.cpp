#include "event_worker.h"
#include "event_queue.h"

#include <exception>
#include <iostream>
#include <string>
#include <thread>

void ProduceEvents(EventQueue& queue, const std::string& producerName, int startIndex) {
    for (int index = 0; index < 3; ++index) {
        PortEvent event {
            "Ethernet" + std::to_string(startIndex + index * 4),
            producerName + "上报链路状态变化"
        };

        queue.Push(std::move(event));
    }
}
int main() {
    EventQueue queue;

    std::thread producerA(ProduceEvents, std::ref(queue), "link-monitor-A", 0);
    std::thread producerB(ProduceEvents, std::ref(queue), "link-monitor-B", 12);

    producerA.join();
    producerB.join();

    std::cout << "两个生产线程已结束，待处理事件数: "
              << queue.Size() << "\n\n";
    
    PortEvent event;
    while (queue.TryPop(event))
    {
        std::cout << "端口：" << event.portName
                  << " | 事件：" << event.description << '\n';
    }

    std::cout << "\n队列是否为空: "
              << (queue.Size() == 0 ? "是" : "否") << '\n';
    
    return 0;
}