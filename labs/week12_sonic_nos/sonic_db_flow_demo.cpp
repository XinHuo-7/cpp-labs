#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct Lag {
    std::vector<std::string> members;
};

class DbFlowDemo {
public:
    void PublishLag(std::string name, Lag lag) {
        // 模拟应用把“需要处理的 LAG”发布到 APPL_DB。
        applDb_[std::move(name)] = std::move(lag);
    }

    void SetPortCreated(const std::string& port, bool created) {
        // 模拟 STATE_DB 中的 state=ok：端口对象已创建。
        // 注意：已创建不等于物理链路一定是 up。
        stateDb_[port] = created;
    }

    bool TryProgramLag(const std::string& name) {
        const auto lag = applDb_.find(name);
        if (lag == applDb_.end()) {
            return false; // APPL_DB 中还没有这个 LAG。
        }
        // all_of：只有每个成员端口都已创建，才满足本实验的依赖条件。
        const bool portsReady = std::all_of(
            lag->second.members.begin(),
            lag->second.members.end(),
            [this](const std::string& port) {
                const auto state = stateDb_.find(port);
                return state != stateDb_.end() && state->second;
            }
        );
        if (!portsReady) {
            return false; // 保留 APPL_DB 对象，等待依赖条件满足。
        }
        // 仅模拟“向下游生成对象”；真实 SONiC 不会这样直接复制。
        asicDb_[name] = lag->second;
        return true;
    }
    bool HasAsicObject(const std::string& name) const {
        return asicDb_.find(name) != asicDb_.end();
    }
private:
    std::unordered_map<std::string, Lag> applDb_;
    std::unordered_map<std::string, bool> stateDb_;
    std::unordered_map<std::string, Lag> asicDb_;
};

int main() {
    DbFlowDemo demo;
    demo.PublishLag("PortChannel1", {{"Ethernet0", "Ethernet4"}});

    demo.SetPortCreated("Ethernet0", true);
    // 关键测试 1：Ethernet4 尚未就绪，不能继续下发。
    assert(!demo.TryProgramLag("PortChannel1"));
    assert(!demo.HasAsicObject("PortChannel1"));

    demo.SetPortCreated("Ethernet4", true);

    // 关键测试 2：依赖条件补齐，再尝试便可以继续。
    assert(demo.TryProgramLag("PortChannel1"));
    assert(demo.HasAsicObject("PortChannel1"));

    std::cout << "LAG dependency flow passed\n";
}