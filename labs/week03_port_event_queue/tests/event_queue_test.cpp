#include "event_queue.h"

#include <cassert>
#include <string>

int main() {
    EventQueue queue;

    assert(queue.Empty());
    assert(queue.Size() == 0);

    queue.Push({"Ethernet0", LinkState::kUp});
    queue.Push({"Ethernet4", LinkState::kDown});
    queue.Push({"Ethernet8", LinkState::kUp});

    assert(!queue.Empty());
    assert(queue.Size() == 3);

    PortEvent event{"", LinkState::kDown};

    assert(queue.TryPop(event));
    assert(event.portName == "Ethernet0");
    assert(event.targetState == LinkState::kUp);
    assert(queue.Size() == 2);

    assert(queue.TryPop(event));
    assert(event.portName == "Ethernet4");
    assert(event.targetState == LinkState::kDown);

    assert(queue.TryPop(event));
    assert(event.portName == "Ethernet8");
    assert(event.targetState == LinkState::kUp);

    assert(queue.Empty());
    assert(queue.Size() == 0);
    assert(!queue.TryPop(event));
    
    return 0;
}