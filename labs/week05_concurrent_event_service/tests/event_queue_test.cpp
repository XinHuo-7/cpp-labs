#include "event_queue.h"

#include <cassert>
#include <string>
#include <thread>

void TestEmptyQueue() {
    EventQueue queue;
    PortEvent event;

    assert(queue.Size() == 0);
    assert(!queue.TryPop(event));
}

void TestPushAndPopInOrder() {
    EventQueue queue;

    queue.Push({"Ethernet0", "LINK_UP"});
    queue.Push({"Ethernet4", "LINK_DOWN"});

    PortEvent event;

    assert(queue.TryPop(event));
    assert(event.portName == "Ethernet0");
    assert(event.description == "LINK_UP");

    assert(queue.TryPop(event));
    assert(event.portName == "Ethernet4");
    assert(event.description == "LINK_DOWN");

    assert(!queue.TryPop(event));
    assert(queue.Size() == 0);
}

void TestCloseStillAllowsDrainingEvents() {
    EventQueue queue;

    queue.Push({"Ethernet0", "LINK_UP"});
    queue.Push({"Ethernet4", "LINK_DOWN"});

    queue.Close();

    PortEvent event;
    assert(queue.WaitPop(event));
    assert(event.portName == "Ethernet0");

    assert(queue.WaitPop(event));
    assert(event.portName == "Ethernet4");

    assert(!queue.WaitPop(event));
    assert(queue.Size() == 0);
}

void TestClosedQueueRejectsNewEvents() {
    EventQueue queue;
    queue.Close();

    assert(!queue.Push({"Ethernet0", "LINK_UP"}));

    PortEvent event;
    assert(!queue.WaitPop(event));
}

void Produce(EventQueue& queue, const std::string& produceName, int eventCount) {
    for (int index = 0; index < eventCount; ++index) {
        assert(queue.Push({produceName + "-Ethernet" + std::to_string(index),
        "LINK_CHANGED"}));
    }
}

void TestConcurrentProduceAndConsume() {
    EventQueue queue;
    int consumedCount = 0;

    std::thread consumer([&queue, &consumedCount] {
        PortEvent event;

        while (queue.WaitPop(event)) {
            ++consumedCount;
        }
    });

    std::thread producerA(Produce, std::ref(queue), "A", 100);
    std::thread producerB(Produce, std::ref(queue), "B", 100);

    producerA.join();
    producerB.join();

    queue.Close();
    consumer.join();
    assert(consumedCount == 200);
    assert(queue.Size() == 0);
}

// void TestConcurrentPush() {
//     EventQueue queue;

//     std::thread producerA(Produce, std::ref(queue), "A");
//     std::thread producerB(Produce, std::ref(queue), "B");
    
//     producerA.join();
//     producerB.join();

//     assert(queue.Size() == 200);

//     PortEvent event;
//     int poppedCount = 0;
//     while (queue.TryPop(event)) {
//         ++poppedCount;
//     }
//     assert(poppedCount == 200);
//     assert(queue.Size() == 0);

// }

int main() {
    TestEmptyQueue();
    TestPushAndPopInOrder();
    TestCloseStillAllowsDrainingEvents();
    TestClosedQueueRejectsNewEvents();
    TestConcurrentProduceAndConsume();
}