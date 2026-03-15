#include "utils/Logger.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

static std::string timestamp() {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S");
    return oss.str();
}

void Logger::log(const std::string& message, LogLevel level) {
    const char* prefix = "[INFO]";
    if (level == LogLevel::WARN) prefix = "[WARN]";
    if (level == LogLevel::ERR)  prefix = "[ERR] ";
    std::cout << "[" << timestamp() << "] " << prefix << " " << message << std::endl;
}

void Logger::info(const std::string& message)  { log(message, LogLevel::INFO); }
void Logger::warn(const std::string& message)  { log(message, LogLevel::WARN); }
void Logger::error(const std::string& message) { log(message, LogLevel::ERR);  }
