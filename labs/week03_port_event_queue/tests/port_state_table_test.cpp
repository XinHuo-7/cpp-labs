#include "port_state_table.h"

#include <cassert>
#include <string>

int main() {
    PortStateTable stateTable;

    LinkState state{LinkState::kDown};

    assert(stateTable.Size() == 0);
    assert(!stateTable.TryGetState("Ethernet0", state));

    assert(stateTable.RegisterPort("Ethernet0"));
    assert(!stateTable.RegisterPort("Ethernet0"));
    assert(stateTable.Size() == 1);

    assert(stateTable.TryGetState("Ethernet0", state));
    assert(state == LinkState::kDown);

    assert(!stateTable.Apply({"Ethernet12", LinkState::kUp}));
    assert(!stateTable.Apply({"Ethernet0", LinkState::kDown}));

    assert(stateTable.Apply({"Ethernet0", LinkState::kUp}));
    assert(stateTable.TryGetState("Ethernet0", state));
    assert(state == LinkState::kUp);

    return 0;
}