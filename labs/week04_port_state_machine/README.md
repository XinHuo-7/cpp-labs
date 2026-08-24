# Port State Machine

状态：

DOWN -> NEGOTIATING -> UP -> FAULT

有效事件：

- DOWN + ADMIN_ENABLE -> NEGOTIATING
- NEGOTIATING + LINK_DETECTED -> UP
- NEGOTIATING / UP + LINK_LOST -> DOWN
- 非 FAULT 状态 + FAULT_DETECTED -> FAULT
- FAULT + RESET -> DOWN

其他状态与事件组合均不改变状态。

## 构建与运行

```bash
cmake -S . -B build
cmake --build build
./build/port_state_machine