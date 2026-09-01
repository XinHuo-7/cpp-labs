#pragma once

#include "event_queue.h"

#include <atomic>
#include <string>
#include <thread>
#include <cstddef>

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

        std::size_t ProcessedCount() const;

    private:
        void Run();

        std::string workerName_;
        std::thread workerThread_;
        EventQueue eventQueue_;

        std::atomic<std::size_t> processedCount_{0};
        bool started_{false};

};
