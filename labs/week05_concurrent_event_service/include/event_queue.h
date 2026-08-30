#pragma once

#include <cstddef>
#include <mutex>
#include <queue>
#include <string>

struct PortEvent
{
    std::string portName;
    std::string description;
};

class EventQueue {
    public:
        void Push(PortEvent event);
        bool TryPop(PortEvent& outEvent);
        std::size_t Size() const;
    private:
        mutable std::mutex mutex_;
        std::queue<PortEvent> events_;
};
