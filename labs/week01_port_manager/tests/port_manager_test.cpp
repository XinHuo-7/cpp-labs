#include "port_manager.h"

#include <cassert>

int main() {
    PortManager manager;

    assert(manager.AddPort("Ethernet0", 10000));
    assert(!manager.AddPort("Ethernet0", 25000));
    assert(!manager.AddPort("Ethernet4", 0));

    assert(manager.GetPortCount() == 1);

    Port* port = manager.FindPort("Ethernet0");
    assert(port != nullptr);
    assert(port->GetLinkState() == LinkState::kDown);

    // assert(manager.SetLinkState("Ethernet0", LinkState::kUp));
    // assert(port->GetLinkState() == LinkState::kUp);

    // assert(!manager.SetLinkState("Ethernet12", LinkState::kUp));
    assert(manager.FindPort("Ethernet12") == nullptr);
    assert(manager.HandlePortEvent("Ethernet0", PortEvent::kLinkUp));
    assert(port->GetLinkState() == LinkState::kUp);

    assert(!manager.HandlePortEvent("Ethernet0", PortEvent::kLinkUp));

    assert(manager.HandlePortEvent("Ethernet0", PortEvent::kLinkDown));
    assert(port->GetLinkState() == LinkState::kDown);   

    return 0;
}