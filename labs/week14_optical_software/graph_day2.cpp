#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Link {
    std::string from;
    std::string to;
    std::string from_port;
    std::string to_port;
    std::set<int> available_wavelengths;
    int cost;
};

struct PathResult {
    bool found = false;
    long long total_cost = 0;
    std::vector<std::string> nodes;
};

class Graph {
public:
    void add_link(const Link& link) {
        if (link.cost < 0) {
            throw std::invalid_argument("Dijkstra requires non-negative costs");
        } 
        adjacency_[link.from].push_back(link);
        adjacency_.try_emplace(link.to);
    }

    void print_topology() const {
        for (const auto& [node, links] : adjacency_) {
            std::cout << "Node " << node << "\n";
            for (const Link& link : links) {
                std::cout << " " << link.from << ":" << link.from_port
                          << " -> " << link.to << ":" << link.to_port
                          << " cost=" << link.cost
                          << " wavelength={";
                bool first = true;
                for (int wavelength : link.available_wavelengths) {
                    if (!first) std::cout << ",";
                    std::cout << wavelength;
                    first = false;
                }
                std::cout << "}\n";
            }
        }
    }

    PathResult shortest_path(const std::string& source, const std::string& destination) const {
        if (adjacency_.find(source) == adjacency_.end() || adjacency_.find(destination) == adjacency_.end()) {
            return {};
        }
        using QueueItem = std::pair<long long, std::string>;
        std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> pending;

        std::map<std::string, long long> distance;
        std::map<std::string, std::string> previous;
        distance[source] = 0;
        pending.push({0, source});
        // 以A-M-B 10 A-D-B 7为例
        while(!pending.empty()) {
            const auto [cost, node] = pending.top();
            pending.pop();

            // 跳过队列中已经过期的记录。
            if (cost != distance.at(node)) continue;
            // 找到destination 
            if (node == destination) break;
            for (const Link& link : adjacency_.at(node)) {
                const long long new_cost = cost + link.cost;
                const auto old = distance.find(link.to);
                if (old == distance.end() || new_cost < old->second) {
                    distance[link.to] = new_cost;
                    previous[link.to] = node;
                    pending.push({new_cost, link.to});
                }
            }
        }

        const auto end = distance.find(destination);
        if (end == distance.end()) return {};
        PathResult result;
        result.found = true;
        result.total_cost = end->second;

        // 从终点沿 previous 倒推回起点。
        for (std::string node = destination; ; node = previous.at(node)) {
            result.nodes.push_back(node);
            if (node == source) break;
        }
        std::reverse(result.nodes.begin(), result.nodes.end());
        return result;
    }

    std::size_t node_count() const {
        return adjacency_.size();
    }
    std::size_t link_count() const {
        std::size_t count = 0;
        for (const auto& [node, links] : adjacency_) {
            (void)node;
            count += links.size();
        }
        return count;
    }
private:
    std::map<std::string, std::vector<Link>> adjacency_;
};

void print_result(const std::string& source, const std::string& destination, const PathResult& result) {
    std::cout << source << "->" << destination << ":";
    if (!result.found) {
        std::cout << "unreachable\n";
        return;
    }
    for (std::size_t i = 0; i < result.nodes.size(); ++i) {
        if (i != 0) std:: cout << "-";
        std::cout << result.nodes[i];
    }
    std::cout << " cost= " << result.total_cost << '\n';
}

int main() {
    Graph graph;
    graph.add_link({"A", "M", "M1", "W1", {2, 3}, 2});
    graph.add_link({"M", "B", "E1", "M1", {2, 3}, 8});
    graph.add_link({"A", "D", "D1", "W1", {1}, 4});
    graph.add_link({"D", "B", "E1", "D1", {1, 2}, 3});

    graph.print_topology();
    std::cout << "nodes= " << graph.node_count() << " "
              << "links=" << graph.link_count() << "\n";

    struct TestCase {
        std::string source;
        std::string destination;
        bool found;
        long long total_cost;
        std::vector<std::string> nodes;
    };
    const std::vector<TestCase> cases = {
        {"A", "B", true, 7, {"A", "D", "B"}},
        {"A", "M", true, 2, {"A", "M"}},
        {"B", "A", false, 0, {}},
        {"A", "A", true, 0, {"A"}}
    };
    for (const TestCase& test: cases) {
        const PathResult actual = graph.shortest_path(test.source, test.destination);
        print_result(test.source, test.destination, actual);
        if (actual.found != test.found || actual.nodes != test.nodes) {
            std::cerr << "Test failed\n";
            return EXIT_FAILURE;
        }
    }
    std::cout << "All " << cases.size() << " tests passed.\n";
    return EXIT_SUCCESS;
}