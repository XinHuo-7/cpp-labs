#include "event_log.h"

#include <stdexcept>

EventLog::EventLog(const std::string& filePath)
        : file_(filePath, std::ios::out | std::ios::app) {
            if(!file_.is_open()) {
                throw std::runtime_error("无法打开日志文件: " + filePath);
            }
}

void EventLog::Write(const std::string& message) {
    file_ << message << '\n';

    if (!file_) {
        throw std::runtime_error("写入日志文件失败");
    }
}