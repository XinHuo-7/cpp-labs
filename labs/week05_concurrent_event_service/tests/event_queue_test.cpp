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

// void TestCloseStilAllowDrainingEvents() {
//     EventQueue queue;

//     queue.Push({"Ethernet0", "LINK_UP"});
//     queue.Push({"Ethernet4", "LINK_DOWN"});

//     queue.Close();

//     PortEvent event;
//     assert
// }

void Produce(EventQueue& queue, const std::string& produceName) {
    for (int index = 0; index < 100; ++index) {
        queue.Push({produceName + "-Ethernet" + std::to_string(index),
        "LINK_CHANGED"});
    }
}

void TestConcurrentPush() {
    EventQueue queue;

    std::thread producerA(Produce, std::ref(queue), "A");
    std::thread producerB(Produce, std::ref(queue), "B");
    
    producerA.join();
    producerB.join();

    assert(queue.Size() == 200);

    PortEvent event;
    int poppedCount = 0;
    while (queue.TryPop(event)) {
        ++poppedCount;
    }
    assert(poppedCount == 200);
    assert(queue.Size() == 0);

}

int main() {
    TestEmptyQueue();
    TestPushAndPopInOrder();
    TestConcurrentPush();
}