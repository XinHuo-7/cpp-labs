#include "event_log.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

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

    std::filesystem::remove(filePath);
    return 0;
}