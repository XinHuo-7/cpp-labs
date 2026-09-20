#pragma once

#include "io_utils.h"

#include <cstdint>

namespace net {

class TimerFd {
    public:
        // 创建定时器，但暂时不启动计时。
        TimerFd();

        // 析构成员 fd_，自动关闭定时器描述符。
        ~TimerFd() = default;
        TimerFd(const TimerFd&) = delete;
        TimerFd& operator = (const TimerFd&) = delete;

        int GetFd() const noexcept {
            return fd_.Get();
        }
        // firstDelayMs：距离第一次到期的时间，必须大于 0。
        // intervalMs：后续重复间隔；0 表示只触发一次。
        void Start(int firstDelayMs, int intervalMs = 0);

        // 停止计时，但保留fd,可以再次 Start
        void Stop();

        // 读取并消费累计到期次数。
        // 当前没有待消费的到期次数时返回 0，不阻塞。
        std::uint64_t Consume();

    private:
        UniqueFd fd_;
};


}