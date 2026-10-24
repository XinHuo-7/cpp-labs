#include <cstddef>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

struct Link {
    std::string from;
    std::string to;
    std::string from_port;
    std::string to_port;
    std::set<int> available_wavelengths;
};

class Graph {
public:
    void add_link(const Link& link) {
        adjacency_[link.from].push_back(link);
        adjacency_.try_emplace(link.to);
    }

    void print_topology() const {
        for (const auto& [node, links] : adjacency_) {
            std::cout << "Node " << node << "\n";
            for (const Link& link : links) {
                std::cout << " " << link.from << ":" << link.from_port
                          << " -> " << link.to << ":" << link.to_port
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

int main() {
    Graph graph;
    graph.add_link({"A", "M", "M1", "W1", {2, 3}});
    graph.add_link({"M", "B", "E1", "M1", {2, 3}});
    graph.add_link({"A", "D", "D1", "W1", {1}});
    graph.add_link({"D", "B", "E1", "D1", {1, 2}});

    graph.print_topology();
    std::cout << "nodes= " << graph.node_count() << " "
              << "links=" << graph.link_count();
}