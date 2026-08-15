#pragma once

#include <fstream>
#include <string>

class EventLog {
public:
    explicit EventLog(const std::string& filePath);

    EventLog(const EventLog&) = delete;
    EventLog& operator=(const EventLog&) = delete;

    void Write(const std::string& message);

private:
    std::ofstream file_;

};