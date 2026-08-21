#include "port.h"

#include <iostream>

void PrintVectorState(const std::vector<Port>& ports) {
    std::cout << "size: " << ports.size()
              << " | capacity: " << ports.capacity()
              << '\n';
}

void RunReallocationDemo() {
    std::vector<Port> ports;

    ports.reserve(3);
    const std::size_t initialCapacity = ports.capacity();

    for (std::size_t index = 0; index < initialCapacity; ++index) {
        ports.emplace_back (
            "Ethernet" + std::to_string(index * 4),
            10000
        );
    }

    PrintVectorState(ports);

    Port* cachedPort = &ports.front();
    std::cout << "扩容前缓存指针指向: "
              << cachedPort->name << "\n";

    const std::size_t capacityBefore = ports.capacity();

    ports.emplace_back("EthernetOverflow", 100000);

    PrintVectorState(ports);

    // std::cout << "错误示例: "
    //       << cachedPort->speedMbps
    //       << '\n';

    if (ports.capacity() > capacityBefore) {
        std::cout << "发生扩容: cachedPort已失效, 不再能访问。\n";
    }

    std::cout << "扩容后第一个端口:"
              << ports.front().name << '\n';

    
}