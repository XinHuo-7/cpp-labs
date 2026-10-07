#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// T 是元素类型，Predicate 是筛选条件的类型。
// 调用时通常不必写出它们；编译器会根据实参推导。
template <typename T, typename Predicate>
std::vector<T> Filter(const std::vector<T>& items, Predicate keep) {
    std::vector<T> result;

    // const T&：逐个读取原元素，不在循环时复制它。
    for(const T& item : items) {
        if(keep(item)) {
            // 符合条件的元素复制到结果容器中。
            result.push_back(item);
        }
    }
    return result;
}

int main() {
    const std::vector<int> ports{-1, 0, 80, 65535, 70000};

    // 这里 T 推导为 int；Predicate 推导为这个 lambda 的类型。
    const auto validPorts = Filter(ports, [](int port) { return port >= 0 && port <= 65535;});

    const std::vector<int> expectedPorts{0, 80, 65535};
    assert(validPorts == expectedPorts);

    const std::vector<std::string> events{"LINK_UP", "TIMEOUT", "LINK_DOWN"};

    // 同一个 Filter，这次 T 推导为 std::string。
    const auto linkEvents = Filter(events, [](const std::string& event) {return event.find("LINK_") == 0;});

    const std::vector<std::string> expectedEvents{"LINK_UP", "LINK_DOWN"};
    assert(linkEvents == expectedEvents);

    std::cout << "Day 3 template tests passed\n";

}