#pragma once

#include <string>

enum class LinkState {
    kDown,
    kUp,
};

struct PortEvent
{
    std::string portName;
    LinkState targetState;
};

inline const char* LinkStateToText(LinkState state) {
    return state == LinkState::kUp ? "UP" : "DOWN";
}