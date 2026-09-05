#include "event_worker.h"

#include <cassert>
#include <stdexcept>

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

int main() {
   TestProcessesSubmittedEvents(); 
   TestRejectsEventsAfterStop();
   TestCannotStartTwice();
   TestDestructStopAndJoins();
   TestConcurrentProducersDoNotLoseEvents();
   TestWorkerLifecycle();
   TestStopBeforeStart();
}