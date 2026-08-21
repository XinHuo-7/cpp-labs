#include "demos.h"

#include <string>
#include <iostream>

int main() {
    RunFilterDemo();

    std::cout << "\n================\n\n";

    RunReallocationDemo();
    
    return 0;
}