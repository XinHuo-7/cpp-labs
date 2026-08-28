#pragma once

#include <string>
#include <vector>
#include <cstddef>

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

struct StateTransition
{
    PortState before;
    PortState after;
    PortEvent event;
    TransitionResult result;
};

const char* ToText(PortState state);
const char* ToText(PortEventType event);


class PortStateMachine {
    public:
        explicit PortStateMachine(std::string portName);

        TransitionResult Apply(const PortEvent& event);
        std::size_t ApplyBatch(const std::vector<PortEvent>& events);

        PortState GetState() const;
        const std::string& GetPortName() const;
        const std::vector<StateTransition>& GetHistory() const;

    private:
        std::string portName_;
        PortState state_{PortState::kDown};
        std::vector<StateTransition> history_;
};

