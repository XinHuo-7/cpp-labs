#include "event_worker.h"

#include <cassert>
#include <stdexcept>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

void TestProcessesSubmittedEvents() {
    EventWorker worker{"test-worker"};

    worker.Start();

    assert(worker.Submit({"Ethernet0", "LINK_UP"}));
    assert(worker.Submit({"Ethernet4", "LINK_DOWN"}));
    assert(worker.Submit({"Ethernet8", "SPEED_CHANGED"}));

    worker.Stop();
    worker.Join();
    assert(worker.ProcessedCount() == 3);
}

void TestRejectsEventsAfterStop() {
     EventWorker worker{"test-worker"};

    worker.Start();
    worker.Stop();
    assert(!worker.Submit({"Ethernet0", "LINK_UP"}));

    worker.Join();
    assert(worker.ProcessedCount() == 0);
}

void TestCannotStartTwice() {
    EventWorker worker{"test-worker"};

    worker.Start();
    bool caught = false;
    try
    {
        worker.Start();
    }
    catch(const std::logic_error&)
    {
        caught = true;
    }
    worker.Stop();
    worker.Join();
    assert(caught);
}

void TestDestructStopAndJoins() {
    EventWorker worker{"test-worker"};

    worker.Start();
    assert(worker.Submit({"Ethernet0", "LINK_UP"}));
}

void SubmitManyEvents(EventWorker& worker, const std::string& producerName, int eventCount) {
    for (int index = 0; index < eventCount; ++index) {
        const bool accepted = worker.Submit({producerName + "-Ethernet" + std::to_string(index), "LINK_CHANGED"});
        assert(accepted);
    }
}

void TestConcurrentProducersDoNotLoseEvents() {
    EventWorker worker{"concurrent-test-worker"};
    
    worker.Start();

    constexpr int kEventsPerProducer = 100;

    std::thread producerA(SubmitManyEvents, std::ref(worker), "A", kEventsPerProducer);
    std::thread producerB(SubmitManyEvents, std::ref(worker), "B", kEventsPerProducer);
    std::thread producerC(SubmitManyEvents, std::ref(worker), "C", kEventsPerProducer);
    
    producerA.join();
    producerB.join();
    producerC.join();

    worker.Stop();
    worker.Join();

    constexpr std::size_t kexpectedCount = 300;
    assert(worker.AcceptedCount() == kexpectedCount);
    assert(worker.ProcessedCount() == kexpectedCount);
}

void TestWorkerLifecycle() {
    EventWorker worker{"lifecycle-test-worker"};

    assert(worker.State() == WorkerState::kCreated);
    assert(!worker.Submit({"Ethernet0", "LINK_UP"}));

    worker.Start();

    assert(worker.State() == WorkerState::kRunning);
    assert(worker.Submit({"Ethernet0", "LINK_UP"}));

    worker.Stop();

    assert(!worker.Submit({"Ethernet4", "LINK_DOWN"}));

    worker.Join();

    assert(worker.State() == WorkerState::kStopped);
    assert(worker.AcceptedCount() == 1);
    assert(worker.ProcessedCount() == 1);
}

void TestStopBeforeStart() {
    EventWorker worker{"never-started-worker"};
    worker.Stop();
    worker.Join();

    assert(worker.State() == WorkerState::kStopped);
    assert(!worker.Submit({"Ethernet0", "LINK_UP"}));

    bool caught = false;

        try {
        worker.Start();
    } catch (const std::logic_error&) {
        caught = true;
    }

    assert(caught);

}

void TestRepeatedStopAndJoin() {
    EventWorker worker{"repeated-stop-worker"};

    worker.Start();
    assert(worker.Submit({"Ethernet0", "LINK_CHANGED"}));

    worker.Stop();
    worker.Stop();

    worker.Join();
    worker.Join();

    assert(worker.State() == WorkerState::kStopped);
    assert(worker.AcceptedCount() == 1);
    assert(worker.ProcessedCount() == 1);
}

void TestConcurrentSubmitAndStop() {
    constexpr int kProducerCount = 4;
    constexpr int kEventsPerProducer = 500;
    constexpr int kTotalAttempts = kProducerCount * kEventsPerProducer;

    EventWorker worker{"stress-worker"};
    std::atomic<int> rejectedCount{0};

    worker.Start();

    std::vector<std::thread> producers;
    producers.reserve(kProducerCount);
    for (int producerIndex = 0; producerIndex < kProducerCount; ++producerIndex) {
        producers.emplace_back([&worker, &rejectedCount, producerIndex] {
            for (int eventIndex = 0; eventIndex < kEventsPerProducer; ++eventIndex) {
                PortEvent event{"Ethernet" + std::to_string(producerIndex) + '-' + std::to_string(eventIndex),
                "LINK_CHANGED"};
                if(!worker.Submit(std::move(event))) {
                    rejectedCount++;
                }
            }
        });
    }
    std::thread stopper([&worker] { std::this_thread::sleep_for(std::chrono::milliseconds(1));
        worker.Stop();
    });

    for (auto& producer : producers) {
        producer.join();
    }

    stopper.join();
    worker.Join();
    const std::size_t accepted = worker.AcceptedCount();
    const std::size_t processed = worker.ProcessedCount();
    const std::size_t rejected = static_cast<std::size_t>(rejectedCount.load());

    assert(worker.State() == WorkerState::kStopped);
    // 成功进入队列的事件必须全部处理。
    assert(accepted == processed);
    // 每次投递要么成功，要么被拒绝。
    assert(accepted + rejected == static_cast<std::size_t>(kTotalAttempts));
    
    assert(!worker.Submit({"Ethernet-final", "LINK_CHANGED"}));
}

int main() {
   TestProcessesSubmittedEvents(); 
   TestRejectsEventsAfterStop();
   TestCannotStartTwice();
   TestDestructStopAndJoins();
   TestConcurrentProducersDoNotLoseEvents();
   TestWorkerLifecycle();
   TestStopBeforeStart();
   TestRepeatedStopAndJoin();
   TestConcurrentSubmitAndStop();
}