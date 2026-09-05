#include "event_worker.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <exception>

EventWorker::EventWorker(std::string workerName)
    : workerName_(std::move(workerName)) {
}

// Stop 队列关闭；Join线程关闭
EventWorker::~EventWorker() {
    Stop();
    Join();
}

void EventWorker::Start() {
    if (state_.load() !=WorkerState::kCreated) {
        throw std::logic_error("event worker has already been started");
    }

    workerThread_ = std::thread(&EventWorker::Run, this);
    state_.store(WorkerState::kRunning);
}

bool EventWorker::Submit(PortEvent event) {
    if (state_.load() != WorkerState::kRunning) {
        return false;
    }

    const bool accepted = eventQueue_.Push(std::move(event));
    if (accepted) {
        ++acceptedCount_;
    }
    return accepted;
}

void EventWorker::Stop() {
    const WorkerState currentState = state_.load();

    if (currentState == WorkerState::kStopping || currentState == WorkerState::kStopped) {
        return;
    }

    if (currentState == WorkerState::kCreated) {
        eventQueue_.Close();
        state_.store(WorkerState::kStopped);
        return;
    }

    state_.store(WorkerState::kStopping);
    eventQueue_.Close();

}

WorkerState EventWorker::State() const {
    return state_.load();
}

void EventWorker::Join() {
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

std::size_t EventWorker::ProcessedCount() const {
    return processedCount_.load();
}
std::size_t EventWorker::AcceptedCount() const {
    return acceptedCount_.load();
}

void EventWorker::Run() {
    try {
        PortEvent event;
        
        while (eventQueue_.WaitPop(event)) {
            std::cout << '[' << workerName_ << ']'
                    << "处理端口: " << event.portName
                    << " | 事件：" << event.description <<'\n';
            ++processedCount_;
        }
    } catch (const std::exception& error) {
        std::cerr << '[' << workerName_ << "] 工作线程异常：" << error.what() << '\n';
    } catch (...) {
        std::cerr << '[' << workerName_ << "] 工作线程发生未知异常";
    }

    state_.store(WorkerState::kStopped);

    std::cout << "[" << workerName_ << "] 队列已关闭, 工作线程退出\n";
}

const char* ToText(WorkerState state) {
    switch (state)
    {
        case WorkerState::kCreated:
            return "CREATED";
        case WorkerState::kRunning:
            return "RUNNING";
        case WorkerState::kStopping:
            return "STOPPING";
        case WorkerState::kStopped:
            return "STOPPED";

    }
    return "UNKNOWN";
}

