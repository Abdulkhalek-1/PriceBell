#pragma once

#include <string>

enum class LogLevel { INFO, WARN, ERR };

class Logger {
public:
    static void log(const std::string& message, LogLevel level = LogLevel::INFO);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
};
