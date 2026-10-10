#include <cassert>
#include <charconv>
#include <iostream>
#include <string_view>
#include <system_error>
#include <variant>

enum class ParseError {
    kEmpty,
    kInvalidNumber,
    kOutofRange
};

// variant 同一时刻保存其中一种类型：int 或 ParseError。
using PortResult = std::variant<int, ParseError>;

PortResult ParsePort(std::string_view text) {
    if (text.empty()) {
        return ParseError::kEmpty;
    }

    int port = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [next, error] = std::from_chars(begin, end, port);

    if (error == std::errc::result_out_of_range) {
        return ParseError::kOutofRange;
    }
    if (error != std::errc{} || next != end) {
        return ParseError::kInvalidNumber;
    }
    if (port < 0 || port > 65535) {
        return ParseError::kOutofRange;
    }

    return port; // 成功时，variant 保存 int。
}

int main() {
    const PortResult good = ParsePort("0");
    assert(std::holds_alternative<int>(good));
    assert(std::get<int>(good) == 0);

    const PortResult malformed = ParsePort("80abc");
    assert(std::holds_alternative<ParseError>(malformed));
    assert(std::get<ParseError>(malformed) == ParseError::kInvalidNumber);

    const PortResult oversized = ParsePort("70000");
    assert(std::holds_alternative<ParseError>(oversized));
    assert(std::get<ParseError>(oversized) == ParseError::kOutofRange);

    std::cout << "Day 6 varint tests passed\n";
}