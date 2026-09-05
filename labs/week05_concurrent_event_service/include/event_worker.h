#pragma once

#include "event_queue.h"

#include <atomic>
#include <string>
#include <thread>
#include <cstddef>

enum class WorkerState {
    kCreated,
    kRunning,
    kStopping,
    kStopped
};

const char* ToText(WorkerState state);

class EventWorker {
    public:
        explicit EventWorker(std::string workerName);

        ~EventWorker();

        EventWorker(const EventWorker&) = delete;
        EventWorker& operator = (const EventWorker&) = delete;

        void Start();
        bool Submit(PortEvent event);
        void Stop();
        void Join();

        std::size_t AcceptedCount() const;
        std::size_t ProcessedCount() const;
        WorkerState State() const;

    private:
        void Run();

        std::string workerName_;
        std::thread workerThread_;
        EventQueue eventQueue_;

        std::atomic<std::size_t> processedCount_{0}; // 后台线程实际处理的事件数
        std::atomic<std::size_t> acceptedCount_{0};  // 成功放入队列的事件数
        std::atomic<WorkerState> state_{WorkerState::kCreated};
};
