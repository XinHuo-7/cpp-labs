#pragma once

#include "port.h"
#include "event_log.h"

#include <cstddef>
#include<unordered_map>
#include <memory>

class PortManager {
public:
    bool AddPort(const std::string& name, int speedMbps);
    Port* FindPort(const std::string& name);

    explicit PortManager(std::unique_ptr<EventLog> eventlog = nullptr);
    bool HandlePortEvent(const std::string& name, PortEvent event);
    void PrintAll() const;

    std::size_t GetPortCount() const;
private:
    std::unordered_map<std::string, Port> ports_;
    std::unique_ptr<EventLog> eventlog_;
};