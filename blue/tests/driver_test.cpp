#include <iostream>

#include "hm11/driver.hpp"
int main() {
    hm11::HM11Driver driver;
    driver.run();
    std::cout << "Test passed" << std::endl;
    return 0;
}
