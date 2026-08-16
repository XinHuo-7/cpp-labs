#pragma once

#include <memory>

class PortPolicy {
public:
    bool IsSpeedAllowed(int speedMbps) const;
};

class PortAdmission {
public:
    explicit PortAdmission(std::shared_ptr<const PortPolicy> policy);

    bool CanUseSpeed(int speedMbps) const;

private:
    std::shared_ptr<const PortPolicy> policy_;
};