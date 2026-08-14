#pragma once

#include <string>

enum class LinkState {
    kDown,
    kUp
};

class Port {
public:
    Port(const std::string& name, int speedMbps);

    const std::string& GetName() const;
    int GetSpeedMbps() const;
    LinkState GetLinkState() const;

    void SetLinkState(LinkState state);
    void PrintStatus() const;
private:
    std::string name_;
    int speedMbps_{0};  
    LinkState linkState_{LinkState::kDown};
};

