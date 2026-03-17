// NOTE: This test needs to be registered in CMakeLists.txt (Track D)
#include "utils/Logger.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>

namespace fs = std::filesystem;

static std::string g_tmpDir;

static void setupTmpDir() {
    g_tmpDir = (fs::temp_directory_path() / "pricebell_test_logger").string();
    fs::remove_all(g_tmpDir);
    fs::create_directories(g_tmpDir);
}

// ── Test: log writes to file ────────────────────────────────────────────────

static void test_log_writes_to_file() {
    setupTmpDir();
    Logger::init(g_tmpDir);
    Logger::setLevel(LogLevel::INFO);
    Logger::log("hello_world_test_marker", LogLevel::INFO);

    std::ifstream in(Logger::logPath());
    assert(in.is_open() && "log file should exist after init + log");

    std::string contents((std::istreambuf_iterator<char>(in)),
                          std::istreambuf_iterator<char>());
    assert(contents.find("hello_world_test_marker") != std::string::npos &&
           "log file should contain the logged message");

    std::cout << "  PASS: test_log_writes_to_file" << std::endl;
    fs::remove_all(g_tmpDir);
}

// ── Test: log level filtering ───────────────────────────────────────────────

static void test_log_level_filtering() {
    setupTmpDir();
    Logger::init(g_tmpDir);
    Logger::setLevel(LogLevel::WARN);

    Logger::info("should_not_appear");
    Logger::warn("should_appear_warn");

    // Flush by reading path (which also flushes internally in lastNLines)
    std::string all = Logger::lastNLines(100);
    assert(all.find("should_not_appear") == std::string::npos &&
           "INFO message should be filtered when level=WARN");
    assert(all.find("should_appear_warn") != std::string::npos &&
           "WARN message should appear when level=WARN");

    std::cout << "  PASS: test_log_level_filtering" << std::endl;
    fs::remove_all(g_tmpDir);
}

// ── Test: log rotation ──────────────────────────────────────────────────────

static void test_log_rotation() {
    setupTmpDir();
    Logger::init(g_tmpDir);
    Logger::setLevel(LogLevel::INFO);

    // Write enough data to exceed 5 MB
    std::string bigMsg(1024, 'X'); // 1 KB per message
    for (int i = 0; i < 5200; ++i) {
        Logger::log(bigMsg, LogLevel::INFO);
    }

    std::string oldPath = Logger::logPath() + ".old";
    assert(fs::exists(oldPath) && "pricebell.log.old should exist after rotation");

    std::cout << "  PASS: test_log_rotation" << std::endl;
    fs::remove_all(g_tmpDir);
}

// ── Test: lastNLines ────────────────────────────────────────────────────────

static void test_last_n_lines() {
    setupTmpDir();
    Logger::init(g_tmpDir);
    Logger::setLevel(LogLevel::INFO);

    for (int i = 0; i < 10; ++i) {
        Logger::log("line_" + std::to_string(i), LogLevel::INFO);
    }

    std::string result = Logger::lastNLines(3);
    // Count newlines — should be exactly 2 (3 lines)
    int newlines = 0;
    for (char c : result) {
        if (c == '\n') ++newlines;
    }
    assert(newlines == 2 && "lastNLines(3) should return exactly 3 lines (2 newlines)");
    assert(result.find("line_9") != std::string::npos && "should contain the last line");
    assert(result.find("line_7") != std::string::npos && "should contain third-to-last line");

    std::cout << "  PASS: test_last_n_lines" << std::endl;
    fs::remove_all(g_tmpDir);
}

// ── Main ────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "Running Logger tests..." << std::endl;

    test_log_writes_to_file();
    test_log_level_filtering();
    test_log_rotation();
    test_last_n_lines();

    std::cout << "All Logger tests passed." << std::endl;
    return 0;
}
