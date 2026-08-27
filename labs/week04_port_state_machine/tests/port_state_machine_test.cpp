#include "port_state_machine.h"

#include <cassert>
#include <stdexcept>
#include <utility>

void TestNormalTransition() {
    PortStateMachine port{"Ethernet0"};

    assert(port.GetState() == PortState::kDown);

    assert(port.Apply({PortEventType::kAdminEnable}) == TransitionResult::kChanged);

    assert(port.GetState() == PortState::kNegotiating);

    assert(port.Apply({PortEventType::kLinkDetected}) == TransitionResult::kChanged);

    assert(port.GetState() == PortState::kUp);

}

void TestInvaildEventDoesNotChangeState() {
    PortStateMachine port{"Ethernet4"};

    assert(port.Apply({PortEventType::kLinkDetected}) == TransitionResult::kIgnored);

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

void TestInvalidFaultEventKeepsState() {
    PortStateMachine port{"Ethernet8"};

    port.Apply({PortEventType::kAdminEnable});
    port.Apply({PortEventType::kLinkDetected});
    assert(port.GetState() == PortState::kUp);

    bool thrown = false;
    try {
        port.Apply({PortEventType::kFaultDetected});
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    assert(thrown);
    assert(port.GetState() == PortState::kUp);
}

void TestMovePortNameIntoMachine() {
    std::string portName{"Ethernet12"};

    PortStateMachine port{portName};

    assert(port.GetPortName() == "Ethernet12");
}

void TestFaultResetTransiting() {
    PortStateMachine port{"Ethernet16"};
    assert(port.Apply({PortEventType::kAdminEnable})
           == TransitionResult::kChanged);
    assert(port.Apply({PortEventType::kLinkDetected})
           == TransitionResult::kChanged);
    assert(port.Apply({
               PortEventType::kFaultDetected,
               "crc error threshold exceeded"
           }) == TransitionResult::kChanged);
    assert(port.GetState() == PortState::kFault);
    assert(port.Apply({PortEventType::kReset})
           == TransitionResult::kChanged);
    assert(port.GetState() == PortState::kDown);
    assert(port.Apply({PortEventType::kReset})
           == TransitionResult::kIgnored);
}

void TestTransitionHistory() {
    PortStateMachine port{"Ethernet20"};

    port.Apply({PortEventType::kLinkDetected});  // 忽略
    port.Apply({PortEventType::kAdminEnable});   // 生效
    const auto& history = port.GetHistory();
    assert(history.size() == 2);

    assert(history[0].before == PortState::kDown);
    assert(history[0].after == PortState::kDown);
    assert(history[0].result == TransitionResult::kIgnored);

    assert(history[1].before == PortState::kDown);
    assert(history[1].after == PortState::kNegotiating);
    assert(history[1].result == TransitionResult::kChanged);
}

int main() {
    TestNormalTransition();
    TestInvaildEventDoesNotChangeState();
    TestEmptyPortNameThrows();
    TestInvalidFaultEventKeepsState();
    TestMovePortNameIntoMachine();
    TestFaultResetTransiting();
    TestTransitionHistory();
}