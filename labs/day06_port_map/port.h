#pragma once

#include <string>

class Port{
    public:
        Port(const std::string& name, int speedMbps);

        const std::string& GetName() const;
        void SetLinkUp(bool isUp);
        void PrintStatus() const;
    private:
        std::string name_;
        bool isUp_{false};
        int speedMbps_{0};
};