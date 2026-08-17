#include "event_log.h"
#include "port_manager.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

int main() {
    const std::string logPath{"port_manager_test.log"};

    std::filesystem::remove(logPath);

    {
        auto policy = std::make_shared<PortPolicy>();
        auto eventLog = std::make_unique<EventLog>(logPath);
        PortManager manager{policy, std::move(eventLog)};

        assert(eventLog == nullptr);

        assert(manager.AddPort("Ethernet0", 10000));
        assert(!manager.AddPort("Ethernet0", 25000));
        assert(!manager.AddPort("Ethernet4", 0));
        assert(!manager.AddPort("Ethernet4", 12345));
        assert(manager.GetPortCount() == 1);

        Port* port = manager.FindPort("Ethernet0");
        assert(port != nullptr);
        assert(port->GetLinkState() == LinkState::kDown);

        assert(manager.HandlePortEvent("Ethernet0", PortEvent::kLinkUp));
        assert(port->GetLinkState() == LinkState::kUp);

        assert(!manager.HandlePortEvent("Ethernet0", PortEvent::kLinkUp));

        assert(manager.HandlePortEvent("Ethernet0", PortEvent::kLinkDown));
        assert(port->GetLinkState() == LinkState::kDown);

        assert(!manager.HandlePortEvent("Ethernet12", PortEvent::kLinkUp));
    }

    {
        std::ifstream input{logPath};

        std::string firstLine;
        std::string secondLine;
        std::string thirdLine;

        std::getline(input, firstLine);
        std::getline(input, secondLine);

        assert(firstLine == "Ethernet0: link state changed");
        assert(secondLine == "Ethernet0: link state changed");
        assert(!std::getline(input, thirdLine));
    }

    std::filesystem::remove(logPath);
    return 0;
}