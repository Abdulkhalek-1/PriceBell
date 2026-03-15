#include "utils/Logger.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <deque>
#include <filesystem>

namespace fs = std::filesystem;

// ── Static member definitions ────────────────────────────────────────────────

std::ofstream Logger::s_file;
std::string   Logger::s_logPath;
LogLevel      Logger::s_minLevel = LogLevel::INFO;
std::mutex    Logger::s_mutex;

// ── Helpers ──────────────────────────────────────────────────────────────────

static std::string timestamp() {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S");
    return oss.str();
}

static const char* levelTag(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "[INFO]";
        case LogLevel::WARN: return "[WARN]";
        case LogLevel::ERR:  return "[ERR] ";
    }
    return "[INFO]";
}

// ── Public API ───────────────────────────────────────────────────────────────

void Logger::init(const std::string& logDir) {
    std::lock_guard<std::mutex> lock(s_mutex);
    fs::create_directories(logDir);
    s_logPath = (fs::path(logDir) / "pricebell.log").string();
    s_file.open(s_logPath, std::ios::app);
}

void Logger::setLevel(LogLevel minLevel) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_minLevel = minLevel;
}

void Logger::log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (level < s_minLevel) return;

    std::string line = "[" + timestamp() + "] " + levelTag(level) + " " + message;
    std::cout << line << std::endl;

    if (s_file.is_open()) {
        s_file << line << std::endl;
        rotateIfNeeded();
    }
}

void Logger::info(const std::string& message)  { log(message, LogLevel::INFO); }
void Logger::warn(const std::string& message)  { log(message, LogLevel::WARN); }
void Logger::error(const std::string& message) { log(message, LogLevel::ERR);  }

std::string Logger::logPath() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_logPath;
}

std::string Logger::lastNLines(int n) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_logPath.empty()) return {};

    // Flush before reading
    if (s_file.is_open()) s_file.flush();

    std::ifstream in(s_logPath);
    if (!in.is_open()) return {};

    std::deque<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
        if (static_cast<int>(lines.size()) > n)
            lines.pop_front();
    }

    std::ostringstream oss;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) oss << '\n';
        oss << lines[i];
    }
    return oss.str();
}

// ── Private ──────────────────────────────────────────────────────────────────

void Logger::rotateIfNeeded() {
    // Caller already holds s_mutex
    if (s_logPath.empty() || !s_file.is_open()) return;

    auto size = fs::file_size(s_logPath);
    if (size <= kMaxLogSize) return;

    s_file.close();
    std::string oldPath = s_logPath + ".old";
    fs::rename(s_logPath, oldPath);
    s_file.open(s_logPath, std::ios::app);
}
