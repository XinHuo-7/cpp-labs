#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

// 模拟处理 epoll 返回的一条就绪事件。
// readyFds 中的数字只是测试数据，不是真正打开的 socket。
void HandleReadyEvent(const std::vector<int>& readyFds, std::size_t index) {
    // at() 会检查下标。
    // 下标越界时抛出 std::out_of_range，
    // 不像越界使用 [] 那样产生未定义行为。
    const int fd = readyFds.at(index);
    std::cout << "[event] index= " << index << " fd= " << fd << std::endl;
}

std::size_t ProcessBatch(bool injectFault) {
    const std::vector<int> readyFds{6, 7};

    // 正常情况处理两个元素。
    // 故障模式故意多处理一次，模拟循环边界写错。
    const std::size_t end = readyFds.size() + (injectFault ? 1:0);

    std::size_t processed = 0;
    for (std::size_t index = 0; index < end; ++index) {
        HandleReadyEvent(readyFds, index);
        ++processed;
    }
    return processed;
}
}

int main(int argc, char* argv[]) {
    // argc：命令行参数数量，包含程序名。
    // argv[1]：第一个额外参数；访问前先检查 argc。
    const bool injectFault = argc == 2 && std::string_view(argv[1]) == "--crash";

    if (argc > 2 || (argc == 2 && !injectFault)) {
        std::cerr << "Usage: core_debug_demo [--crash]\n";
        return 2;
    }

    // 仅在这个独立排障程序中，故意不捕获异常。
    // 故障模式会因未捕获异常而终止，便于练习调试。
    const auto processed = ProcessBatch(injectFault);

    // 正常路径的关键检查，不依赖 assert 是否被编译关闭。
    if (processed != 2) {
        std::cerr << "Unexpected processed count\n";
        return 1;
    }

    std::cout << "Normal path passed\n";
    return 0;
}