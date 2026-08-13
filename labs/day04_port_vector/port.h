#pragma once

#include <string>

class Port {
    public:
        Port(const std::string& name, int speedMbps);

        void SetLinkUp(bool isUp);
        void PrintStatus() const;
    private:
        std::string name_;
        bool isUp_{false};
        int speedMbps_{0};
};