#include <cassert>
#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>

// 成功：返回包含端口号的 optional<int>。
// 失败：返回 std::nullopt，而不是用 0 假装失败。
std::optional<int> ParsePort(std::string_view text) {
    if(text.empty()) {
        return std::nullopt;
    }
    int port = 0;
    const char* begin = text.data();
    const char* end = begin +  text.size();

    // from_chars 解析 [begin, end)；next 指向停止解析的位置。char转换为int
    const auto [next, error] = std::from_chars(begin, end, port);

    // 不仅要解析成功，还必须吃完整个字符串。
    // 例如 "80abc" 会停在 'a'，因此也应判为无效。
    if (error != std::errc{} || next != end || port < 0 || port > 65535) {
        return std::nullopt;
    }
    return port; // int 自动放进 optional<int>
}

int main() {
    const auto normal = ParsePort("8080");
    assert(normal.has_value());
    assert(*normal == 8080); // 确认有值后，再用 * 取出 int

    const auto zero = ParsePort("0");
    assert(zero.has_value() && *zero ==0);

    assert(!ParsePort("70000").has_value());
    assert(!ParsePort("80abc").has_value());
    std::cout << "Day 5 optional test passed\n";
}
