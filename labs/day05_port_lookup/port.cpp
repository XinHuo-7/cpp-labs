#include "port.h"

#include <iostream>

Port::Port(const std::string& name, int speedMbps)
    : name_(name), speedMbps_(speedMbps) { 
}

const std::string& Port::GetName() const {
    return name_;
}

void Port::SetLinkUp(bool isUp) {
    isUp_ = isUp;
}

void Port::PrintStatus() const {
    std::cout << "端口: " << name_
              << "| 状态: " << (isUp_ ? "UP" : "DOWN")
              << "| 速率: " << speedMbps_ << "Mbps\n";
}