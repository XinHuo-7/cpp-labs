#include "event_worker.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <utility>

EventWorker::EventWorker(std::string workerName)
    : workerName_(std::move(workerName)) {
}

EventWorker::~EventWorker() {
    Join();
}

void EventWorker::Start() {
    if (workerThread_.joinable()) {
        throw std::logic_error("worker is already running or not joined");
    }

    workerThread_ = std::thread(&EventWorker::Run, this);
}

void EventWorker::Join() {
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void EventWorker::Run() {
    for (int eventId = 1; eventId <= 3; ++eventId) {
        std::cout << "[" << workerName_ << "]"
                  << "处理启动事件 #" << eventId << '\n';
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::cout << "[" << workerName_ << "] 事件处理完成\n";
}