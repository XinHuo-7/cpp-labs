#include <cassert>
#include <iostream>
#include <string>
#include <utility>

enum class ValueKind {
    kLvalue,
    kRvalue
};

ValueKind Inspect(const std::string&) {
    return ValueKind::kLvalue;
}

ValueKind Inspect(const std::string&&) {
    return ValueKind::kRvalue;
}

// T&& 在这里是“转发引用”：T 由调用者传入的实参推导。
// 但 value 有名字，因此在函数体内写 value 时，它是左值表达式。
template <typename T>
ValueKind RelayWithoutForward(T&& value) {
    return Inspect(value);
}

template <typename T>
ValueKind Relay(T&& value) {
    // std::forward<T>：调用者传左值，就继续传左值；
    // 调用者传右值，就继续传右值。它本身不搬运数据。
    return Inspect(std::forward<T>(value));
}

int main() {
    std::string message{"PORT_UP"};

    // 对照组：不使用 forward，两种输入都变成左值。
    assert(RelayWithoutForward(message) == ValueKind::kLvalue);
    assert(RelayWithoutForward(std::string{"PORT_DOWN"}) == ValueKind::kLvalue);

     // 正确转发：保留调用者传入时的类别。
     assert(Relay(message) == ValueKind::kLvalue);
     assert(Relay(std::string{"PORT_DOWN"}) == ValueKind::kRvalue);

     // std::move 把传入表达式变成右值；Inspect 只观察，不移动内容。
     assert(Relay(std::move(message)) == ValueKind::kRvalue);
     assert(message == "PORT_UP");
     std::cout << "Day 2 forwarding tests passed\n";
    return 0;
}