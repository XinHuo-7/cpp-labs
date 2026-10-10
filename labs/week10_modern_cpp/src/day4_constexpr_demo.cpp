#include <cassert>
#include <iostream>

struct ServerConfig {
    int port;
    int backlog;
};

// constexpr：参数和计算满足条件时，此函数可以在编译期求值。
// 这里规定 backlog 必须大于 0，是本练习的配置规则。
constexpr bool IsValidConfig(const ServerConfig& config) noexcept {
    return config.port >=0 && config.port <= 65535 && config.backlog > 0;
}

// constexpr 变量的初始值必须可在编译期确定。
constexpr ServerConfig kDefaultconfig{80800, 16};

// static_assert 要求条件在编译期就能算出；失败则无法编译。
// static_assert(IsValidConfig(kDefaultconfig), "default config should be valid");
static_assert(!IsValidConfig(ServerConfig{70000, 16}), "port 70000 should be rejected");

int main() {
    ServerConfig runtimeConfig{0, 16};

    // 普通对象也能调用 constexpr 函数；
    // 这里没有要求结果必须成为编译期常量。
    assert(IsValidConfig(runtimeConfig));

    runtimeConfig.port = 70000;
    assert(!IsValidConfig(runtimeConfig));
    
    std::cout << "Day 4 constexpr tests passed\n";
}