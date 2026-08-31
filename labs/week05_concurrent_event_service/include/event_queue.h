#pragma once

#include <condition_variable>
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
        bool Push(PortEvent event);

        bool TryPop(PortEvent& outEvent);
        bool WaitPop(PortEvent& outEvent);

        void Close();
        std::size_t Size() const;

    private:
        mutable std::mutex mutex_;
        std::condition_variable eventAvailable_;
        std::queue<PortEvent> events_;
        bool closed_{false};
};
