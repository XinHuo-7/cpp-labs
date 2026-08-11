#include "port.h"

int main() {
    Port uplinkPort{"Ethernet0", 10000};
    uplinkPort.SetLinkUp(true);
    uplinkPort.SetSpeedMbps(25000);
    uplinkPort.PrintStatus();

    return 0;
}