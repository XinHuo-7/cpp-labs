#pragma once

#include "port_event.h"

#include <cstddef>
#include <deque>

class EventQueue {
    public:
        void Push(PortEvent event);
        bool TryPop(PortEvent& outEvent);

        std::size_t Size() const;
        bool Empty() const;
    
    private:
        std::deque<PortEvent> events_;

};