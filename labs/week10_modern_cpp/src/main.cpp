#include <cassert>
#include <iostream>
#include <string>
#include <utility>

// 左值可以绑定到 const 左值引用。
void Inspect(const std::string& value) {
    std::cout << "左值: " << value << '\n';
}

// 临时对象或经过 std::move 转换的表达式可以绑定到右值引用。
void Inspect(std::string && value) {
    std::cout << "右值: " << value << '\n';
}

int main() {
    std::string message{"LINK_CHANGED"};

    Inspect(message);                         // 有名字的对象：左值
    Inspect(std::string{"PORT_DOWN"});        // 临时对象：右值

    // std::move 只转换表达式的值类别；Inspect 只是读取，并没有移动内容。
    Inspect(std::move(message));
    assert(message == "LINK_CHANGED");

    // 虽然 ref 的类型是 std::string&&，但有名字的 ref 表达式仍是左值。
    std::string&& ref = std::string{"PORT_UP"};
    Inspect(ref);
    Inspect(std::move(ref));

    std::string copied = message;            // 从左值构造：复制
    std::string moved = std::move(message);  // 从右值表达式构造：移动
    assert(copied == "LINK_CHANGED");
    assert(moved == "LINK_CHANGED");

     // 移动后的 message 仍是有效对象，但不要假定它的内容一定为空。
     message = "RESET";
     assert(message == "RESET");

    std::cout << "Day 1 checks passed\n";

}