#include "event_worker.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <utility>

EventWorker::EventWorker(std::string workerName)
    : workerName_(std::move(workerName)) {
}

EventWorker::~EventWorker() {
    Stop();
    Join();
}

void EventWorker::Start() {
    if (started_) {
        throw std::logic_error("event worker has already been started");
    }

    workerThread_ = std::thread(&EventWorker::Run, this);
    started_ = true;
}

bool EventWorker::Submit(PortEvent event) {
    return eventQueue_.Push(std::move(event));
}

void EventWorker::Stop() {
    eventQueue_.Close();
}

void EventWorker::Join() {
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

std::size_t EventWorker::ProcessedCount() const {
    return processedCount_.load();
}

void EventWorker::Run() {
    PortEvent event;

    while (eventQueue_.WaitPop(event)) {
        std::cout << '[' << workerName_ << ']'
                  << "处理端口: " << event.portName
                  << " | 事件：" << event.description <<'\n';
        ++processedCount_;
    }

    std::cout << "[" << workerName_ << "] 队列已关闭, 工作线程退出\n";
}