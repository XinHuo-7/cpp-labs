#pragma once

#include <string>

enum class PortState {
    kDown,
    kNegotiating,
    kUp,
    kFault,
};

enum class PortEventType {
    kAdminEnable,
    kLinkDetected,
    kLinkLost,
    kFaultDetected,
    kReset,
};

enum class TransitionResult {
    kChanged,
    kIgnored,
};

struct PortEvent
{
    PortEventType type;
    std::string reason{};
};


const char* ToText(PortState state);
const char* ToText(PortEventType event);

class PortStateMachine {
    public:
        explicit PortStateMachine(std::string portName);

        TransitionResult Apply(const PortEvent& event);

        PortState GetState() const;
        const std::string& GetPortName() const;

    private:
        std::string portName_;
        PortState state_{PortState::kDown};
};

