#include <cstddef>
#include <set>
#include <vector>

std::set<int> intersection(const std::set<int>& a,const std::set<int>& b) {
    std::set<int> result;
    for (int wavelength : b) {
        if (a.count(wavelength) != 0) {
            result.insert(wavelength);
        }
    }
    return result;
}

std::set<int> commonWavelengths(
    const std::vector<std::set<int>>& hops
) {
    if (hops.empty()) {
        return {};
    }

    std::set<int> result = hops[0];
    for (std::size_t i = 1; i < hops.size(); ++i) {
        result = intersection(result, hops[i]);
        if (result.empty()) {
            break;
        }
    }
    return result;
}