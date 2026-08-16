#include "port_policy.h"

#include <cassert>
#include <memory>

int main() {
    auto policy = std::make_shared<PortPolicy>();

    PortAdmission lineCardA{policy};
    PortAdmission lineCardB{policy};

    assert(lineCardA.CanUseSpeed(10000));
    assert(lineCardB.CanUseSpeed(100000));
    assert(!lineCardA.CanUseSpeed(12345));

    policy.reset();

    assert(lineCardA.CanUseSpeed(25000));
    assert(lineCardB.CanUseSpeed(1000));
    return 0;
}