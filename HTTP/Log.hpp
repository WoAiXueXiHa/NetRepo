#pragma once
#include <iostream>
#include <string>
#include <ctime>

enum LogLevel {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// 简单的日志宏，实际项目可以用 log4cpp 或 spdlog
#define LOG(level) std::cout << "[" #level "] " << __FILE__ << ":" << __LINE__ << " " << std::time(nullptr) << " - "