#include <set>
#include <vector>

// std::set<int> 保存不重复的波长编号。
// hops 中的每个 set 代表一段链路的可用波长。
std::set<int> intersection(std::set<int>& hopA, std::set<int>& hopB) {
    std::set<int> finalHop;
    for (int value: hopB) {
        if(hopA.count(value)) {
            finalHop.insert(value);
        }
    }
    return finalHop;
}

std::set<int> commonWavelengths(const std::vector<std::set<int>>& hops) {
    std::set<int> finalHop = hops[0];
    for (auto hop : hops) {
        finalHop = intersection(finalHop, hop);
    }
    return finalHop;
}