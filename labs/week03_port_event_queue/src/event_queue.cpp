#include "event_queue.h"

#include <utility>

void EventQueue::Push(PortEvent event) {
    events_.push_back(std::move(event));
}

bool EventQueue::TryPop(PortEvent& outEvent) {
    if(events_.empty()) {
        return false;
    }

    outEvent = std::move(events_.front());
    events_.pop_front();

    return true;
}

std::size_t EventQueue::Size() const {
    return events_.size();
}

bool EventQueue::Empty() const {
    return events_.empty();
}