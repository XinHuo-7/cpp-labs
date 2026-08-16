#include "port_policy.h"

#include <utility>

bool PortPolicy::IsSpeedAllowed(int speedMbps) const {
    return speedMbps == 1000
        || speedMbps == 10000
        || speedMbps == 25000
        || speedMbps == 100000;
}

PortAdmission::PortAdmission(
    std::shared_ptr<const PortPolicy> policy)
    : policy_(std::move(policy)) {
}

bool PortAdmission::CanUseSpeed(int speedMbps) const {
    return policy_ != nullptr
        && policy_->IsSpeedAllowed(speedMbps);
}