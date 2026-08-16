#include "event_log.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>

int main() {
    const std::string filePath("event_log_test.log");

    std::filesystem::remove(filePath);
    {
        EventLog log{filePath};
        log.Write("Ethernet0: DOWN -> UP");
    }

    std::ifstream input{filePath};
    std::string line;
    std::getline(input, line);

    assert(line == "Ethernet0: DOWN -> UP");

    bool threw{false};
    try {
        EventLog invalidLog{
            "missing_log_directory_for_test/port_events.log"
        };
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    std::filesystem::remove(filePath);
    return 0;
}