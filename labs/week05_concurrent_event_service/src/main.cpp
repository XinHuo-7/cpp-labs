#include "event_worker.h"

#include <exception>
#include <iostream>

int main() {
    try {
        EventWorker worker{"port-event-worker"};

        worker.Start();

        std::cout << "[main] 后台工作线程已启动，等待其完成\n";

        worker.Join();

        std::cout << "[main] 后台工作线程已回收，程序正常结束\n";
    } catch (const std::exception& error) {
        std::cerr << "[main] 程序失败: "
                  << error.what() << '\n';
        return 1;
    }

    return 0;
}