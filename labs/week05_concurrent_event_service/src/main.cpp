#include "event_worker.h"
#include "event_queue.h"

#include <exception>
#include <iostream>
#include <string>
#include <thread>

void SubmitEvents(EventWorker& worker, const std::string& producerName, int startIndex) {
    for (int index = 0; index < 3; ++index) {
        PortEvent event{
            producerName + "-Ethernet" + std::to_string(startIndex + index),
            "LINK_CHANGED"
        };
        if (!worker.Submit(std::move(event))) {
            std::cerr << '[' << producerName << "] 事件投递失败\n";
            return;
        }
    }
}

void ProduceEvents(EventQueue& queue, const std::string& producerName, int startIndex) {
    for (int index = 0; index < 3; ++index) {
        PortEvent event {
            "Ethernet" + std::to_string(startIndex + index * 4),
            producerName + "上报链路状态变化"
        };

        if(!queue.Push(std::move(event))) {
            std::cerr << "事件队列已关闭，事件投递失败\n";
            return;
        }
    }
}

void ConsumeEvents(EventQueue& queue, int& consumedCount) {
    PortEvent event;

    while (queue.WaitPop(event))
    {
        std::cout << "端口: " << event.portName
                  << " | 事件: " << event.description << '\n';
        ++consumedCount;
    }

    std::cout << "[consumer] 队列已关闭，消费者退出\n";
}

// day3 多线程调度
// int main() {
//     EventQueue queue;
//     int consumedCount = 0;

//     std::thread consumer(ConsumeEvents, std::ref(queue), std::ref(consumedCount));
//     std::thread producerA(ProduceEvents, std::ref(queue), "link-monitor-A", 0);
//     std::thread producerB(ProduceEvents, std::ref(queue), "link-monitor-B", 12);

//     producerA.join();
//     producerB.join();

//     std::cout << "[main]两个生产线程已结束，关闭事件队列\n";

//     queue.Close();
    
//     consumer.join();
    
//     std::cout << "[main] 已消费事件数: " << consumedCount << '\n';
//     return 0;
// }

int main() {
    EventWorker worker{"port-event-worker"};

    worker.Start();
    std::thread producerA(SubmitEvents, std::ref(worker), "LACP", 0);
    std::thread producerB(SubmitEvents, std::ref(worker), "BGP", 100);
    std::thread producerC(SubmitEvents, std::ref(worker), "PORT", 200);

    producerA.join();
    producerB.join();
    producerC.join();

    worker.Stop();
    worker.Join();

    const std::size_t accepted = worker.AcceptedCount();
    const std::size_t processed = worker.ProcessedCount();

    std::cout << "[main] 已处理事件数：" << accepted << '\n';
    std::cout << "[main] 实际处理事件数：" << processed << '\n';

    if (accepted != processed) {
        std::cerr << "[main] 检测到事件丢失\n";
        return 1;
    }
    std::cout << "[main] 所有事件处理完成";
    return 0;
}