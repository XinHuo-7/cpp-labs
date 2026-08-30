#include "event_queue.h"

#include <utility>

void EventQueue::Push(PortEvent event) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push(std::move(event));
}

bool EventQueue::TryPop(PortEvent& outEvent) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (events_.empty()) {
        return false;
    }

    outEvent = std::move(events_.front());
    events_.pop();
    return true;
}

std::size_t EventQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
}