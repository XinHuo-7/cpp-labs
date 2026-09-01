#include "event_queue.h"

#include <utility>

bool EventQueue::Push(PortEvent event) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return false;
        }

        events_.push(std::move(event));
    }

    eventAvailable_.notify_one();
    return true;
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

bool EventQueue::WaitPop(PortEvent& outEvent) {
    std::unique_lock<std::mutex> lock(mutex_);

    eventAvailable_.wait(lock, [this] {return closed_ || !events_.empty();});

    if (events_.empty()) {
        return false;
    }

    outEvent = std::move(events_.front());
    events_.pop();
    return true;
}

void EventQueue::Close() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }
    eventAvailable_.notify_all();
}

std::size_t EventQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
}