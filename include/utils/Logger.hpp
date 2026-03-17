#pragma once
#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel { INFO, WARN, ERR };

class Logger {
public:
    static void init(const std::string& logDir);
    static void setLevel(LogLevel minLevel);
    static void log(const std::string& message, LogLevel level = LogLevel::INFO);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static std::string logPath();
    static std::string lastNLines(int n);

private:
    static void rotateIfNeeded();

    static std::ofstream s_file;
    static std::string   s_logPath;
    static LogLevel      s_minLevel;
    static std::mutex    s_mutex;
    static constexpr size_t kMaxLogSize = 5 * 1024 * 1024; // 5 MB
};
