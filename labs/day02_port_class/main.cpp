#include <iostream>
#include <string>

class Port {
    public:
        Port(const std::string& name, int speedMbps)
            : name_(name), speedMbps_(speedMbps) {
        }

        void SetLinkup(bool isUp) {
            isUp_ = isUp;
        }

        void SetSpeedMbps(int speedMbps) {
            if (speedMbps > 0) {
                speedMbps_ = speedMbps;
            }
        }

        void PrintStatus() const {
            std::cout << "端口: " << name_ << '\n';
            std::cout << "状态: " << (isUp_ ? "UP" : "DOWN") << '\n';
            std::cout << "速率: " << speedMbps_ << " Mbps\n";
        }
        private:
            std::string name_;
            bool isUp_{false};
            int speedMbps_{0};
};

int main() {
    Port uplinkPort{"Ethernet0", 10000};

    uplinkPort.SetLinkup(true);
    uplinkPort.SetSpeedMbps(-1);
    uplinkPort.PrintStatus();

    return 0;
}