#include "utils/Logger.hpp"
#include <iostream>

void Logger::log(const std::string& message) {
    std::cout << "[LOG] " << message << std::endl;
}
