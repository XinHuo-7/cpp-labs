#include "port.h"

#include <iostream>

Port::Port(const std::string& name, int speedMbps)
    : name_(name), speedMbps_(speedMbps) {
}

void Port::SetLinkUp(bool isUp) {
    isUp_ = isUp;
}

void Port::SetSpeedMbps(int speedMbps) {
    if (speedMbps > 0) {
        speedMbps_ = speedMbps;
    }
}

void Port::PrintStatus() const {
    std::cout << "端口: " << name_ << '\n';
    std::cout << "状态: " << (isUp_ ? "UP" : "DOWN") << '\n';
    std::cout << "速率: " << speedMbps_ << " Mbps\n";
}