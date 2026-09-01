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

int main() {
   TestProcessesSubmittedEvents(); 
   TestRejectsEventsAfterStop();
   TestCannotStartTwice();
   TestDestructStopAndJoins();
}