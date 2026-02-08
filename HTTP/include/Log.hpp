#pragma once

#include <iostream>
#include <string>
#include <ctime>

namespace LogModule {

    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    // 获取当前时间的小工具 (时:分:秒)
    inline std::string GetTime() {
        char buffer[64];
        time_t t = time(nullptr);
        strftime(buffer, sizeof(buffer), "%H:%M:%S", localtime(&t));
        return buffer;
    }

    // [极简宏] 核心逻辑
    // 技巧：#level 是宏的字符串化操作
    // 如果你写 LOG(INFO)，#level 就是 "INFO"
    // 如果你写 LOG(ERROR)，#level 就是 "ERROR"
    #define LOG(level) std::cout << "[" << LogModule::GetTime() << "] [" << #level << "] "

}