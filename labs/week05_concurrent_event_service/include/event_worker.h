#pragma once

#include <string>
#include <thread>

class EventWorker {
    public:
        explicit EventWorker(std::string workerName);

        ~EventWorker();

        EventWorker(const EventWorker&) = delete;
        EventWorker& operator = (const EventWorker&) = delete;

        void Start();
        void Join();

    private:
        void Run();

        std::string workerName_;
        std::thread workerThread_;

};
