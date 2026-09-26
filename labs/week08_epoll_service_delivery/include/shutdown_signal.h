#pragma once

namespace net {

// 安装 SIGINT 和 SIGTERM 的处理函数。
// SIGINT：通常由 Ctrl+C 产生。
// SIGTERM：通常由服务管理程序要求进程结束时产生。
void InstallShutdownSignalHandlers();

// 查询是否已经收到停止信号。
bool IsShutdownRequested() noexcept;

} // namespace net