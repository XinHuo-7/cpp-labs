#include "port_state_machine.h"

#include <cassert>
#include <stdexcept>

void TestNormalTransition() {
    PortStateMachine port{"Ethernet0"};

    assert(port.GetState() == PortState::kDown);

    assert(port.Apply(PortEvent::kAdminEnable) == TransitionResult::kChanged);

    assert(port.GetState() == PortState::kNegotiating);

    assert(port.Apply(PortEvent::kLinkDetected) == TransitionResult::kChanged);

    assert(port.GetState() == PortState::kUp);

}

void TestInvaildEventDoesNotChangeState() {
    PortStateMachine port{"Ethernet4"};

    assert(port.Apply(PortEvent::kLinkDetected) == TransitionResult::kIgnored);

    assert(port.GetState() == PortState::kDown);
}

void TestEmptyPortNameThrows() {
    bool thrown = false;
    try
    {
        PortStateMachine invalidPort{""};
    }
    catch(const std::invalid_argument&)
    {
        thrown = true;
    }
    assert(thrown);   
}

int main() {
    TestNormalTransition();
    TestInvaildEventDoesNotChangeState();
    TestEmptyPortNameThrows();
}