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
```

## 批量事件处理

`ApplyBatch()` 使用两阶段提交：

1. 在临时状态和临时历史中模拟全部事件。
2. 全部成功后，一次性提交历史与最终状态。

因此，批次中任意事件非法时，端口状态和历史记录都不会改变。
