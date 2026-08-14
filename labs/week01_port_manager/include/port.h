#pragma once

#include <string>

enum class LinkState {
    kDown,
    kUp
};

enum class PortEvent {
    kLinkUp,
    kLinkDown
};


class Port {
public:
    Port(const std::string& name, int speedMbps);

    const std::string& GetName() const;
    int GetSpeedMbps() const;
    LinkState GetLinkState() const;

    bool HandleEvent(PortEvent event);
    void PrintStatus() const;
private:
    std::string name_;
    int speedMbps_{0};  
    LinkState linkState_{LinkState::kDown};
};

