#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct Event {
    std::string name;
    int port;
};

struct Summary {
    std::size_t matchedCount;
    std::optional<int> firstPort;
    std::chrono::microseconds elapsed;
};

// std::function<bool(const Event&)> 表示：接收 Event，返回 bool 的筛选规则。
// 这里可以传入符合该签名的 lambda。

Summary Analyze(const std::vector<Event>& events, const std::function<bool(const Event&)>& matches) {
    // steady_clock 适合测量经过的时间，而不是表示日历时间。
    const auto start = std::chrono::steady_clock::now();

    // count_if：统计满足规则的元素数量。
    const auto count = std::count_if(events.begin(), events.end(), matches);

    // find_if：返回第一个满足规则的迭代器；
    // 找不到时返回 events.end()。
    const auto first = std::find_if(events.begin(), events.end(), matches);

    std::optional<int> firstPort;
    if (first != events.end()) {
        firstPort = first->port;
    }

    // 两个时间点相减得到时长，再换算为微秒。
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start);
    return {static_cast<std::size_t>(count), firstPort, elapsed};
}

int main() {
    const std::vector<Event> events {
        {"TIMEOUT", 1},
        {"LINK_CHANGED", 10},
        {"LINK_CHANGED", 11}
    };

    const std::function<bool(const Event&)> isLinkChanged = [](const Event& event) {
        return event.name == "LINK_CHANGED";
    };

    // 关键测试 1：有两个匹配项，第一个匹配端口是 10。
    const Summary found = Analyze(events, isLinkChanged);
    assert(found.matchedCount == 2);
    assert(found.firstPort.has_value());
    assert(*found.firstPort == 10);

    // 关键测试 2：空列表没有匹配项，也没有“第一个端口”。
    const std::vector<Event> empty;
    const Summary notFound = Analyze(empty, isLinkChanged);
    assert(notFound.matchedCount == 0);
    assert(!notFound.firstPort.has_value());

    // 耗时受机器和运行环境影响，不检查它等于某个固定数字。
    std::cout << "matched=" << found.matchedCount
              << ", firstPort=" << *found.firstPort
              << ", eplased_us=" << found.elapsed.count() << '\n';
    std::cout << "week 10 tests passed\n";
}