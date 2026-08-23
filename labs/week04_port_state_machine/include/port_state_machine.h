#pragma once

#include <string>

enum class PortState {
    kDown,
    kNegotiating,
    kUp,
    kFault,
};

enum class PortEvent {
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

const char* ToText(PortState state);
const char* ToText(PortEvent event);

class PortStateMachine {
    public:
        explicit PortStateMachine(std::string portName);

        TransitionResult Apply(PortEvent event);

        PortState GetState() const;
        const std::string& GetPortName() const;

    private:
        std::string portName_;
        PortState state_{PortState::kDown};
};

