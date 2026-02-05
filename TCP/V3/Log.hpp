#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>
#include <unistd.h>
#include <time.h>
#include "Lock.hpp"

namespace LogModule{
    using namespace LockModule;
    // ----  工具函数：获取当前格式化时间 ----
    std::string CurTime(){
        time_t time_cur = ::time(nullptr);
        struct tm cur;
        // 可重入版本，保证线程安全
        localtime_r(&time_cur, &cur);
        char buf[1024];
        // YYYY-MM-DDD HH:MM::SS
        snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
                      cur.tm_year + 1900,           
                      cur.tm_mon + 1,
                      cur.tm_mday,
                      cur.tm_hour,
                      cur.tm_min,
                      cur.tm_sec
                );
        return buf;
    }

    // ---- 默认配置 ----
    const std::string defaultLogPath = "./Log/";
    const std::string defaultLogName = "log.txt";

    // ---- 日志等级 ----
    enum LogLevel{
        DEBUG = 1, 
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    // ---- 辅助函数：将枚举转为字符串 ----
    std::string Level2String(LogLevel level){
        switch(level){
            case DEBUG : return "DEBUG";
            case INFO : return "INFO";
            case WARNING : return "WARNING";
            case ERROR : return "ERROR";
            case FATAL : return "FATAL";
            default : return "NONE";
        }
    }

    // ---- 策略模式基类 ----
    class LogStrategy{
    public:
        // 析构必须时virtual，调用基类指针时，执行基类析构，不会执行派生类析构
        // 如果派生类有申请的资源得不到释放，会导致资源泄露
        virtual ~LogStrategy() = default;
        // 纯虚函数强制派生类重写刷新方法
        virtual void SyncLog(const std::string& msg) = 0;
    };

    // ---- 策略1： 控制台输出
    class ConsoleLogStrategy : public LogStrategy{
    public:
        ConsoleLogStrategy() {}
        ~ConsoleLogStrategy() {}
        void SyncLog(const std::string& msg){
            LockGuard lockGuard(_lock);
            std::cout << msg << std::endl;
        }
    private:
        Mutex _lock;
    };

    // ---- 策略2：文件输出 ----
    class FileLogStrategy : public LogStrategy{
    public:
        FileLogStrategy(const std::string logPath = defaultLogPath, const std::string logName = defaultLogName)
            :_logPath(logPath)
            ,_logName(logName)
            {
                LockGuard lockGuard(_lock);
                // 路径存在则返回，不存在创建新的路径
                if(std::filesystem::exists(_logPath)) return;
                try {
                    std::filesystem::create_directories(_logPath);
                }
                catch(std::filesystem::filesystem_error& e) {
                    std::cerr << e.what() << "\n";
                }
            }
        ~FileLogStrategy() {}
        
        void SyncLog(const std::string& msg){
            LockGuard lockGuard(_lock);
            std::string log = _logPath + _logName;
            std::ofstream out(log, std::ios::app);
            if(!out.is_open()) return;
            out << msg << "\n";
            out.close();
        }
    private:
        Mutex _lock;
        std::string _logPath = defaultLogPath;
        std::string _logName = defaultLogName;
    };

    // ---- 核心日志类 ----
    class Logger{
    public:
        Logger(){
            // 默认往控制台输出
            _strategy = std::make_shared<ConsoleLogStrategy>();
        }

        void EnableConsoleLog(){
            _strategy = std::make_shared<ConsoleLogStrategy>();
        }
        void EnableFileLog(){
            _strategy = std::make_shared<FileLogStrategy>();
        }
        ~Logger(){}

        // 内部类：LogMsg
        // 收集一条日志的所有信息，一个对象代表一条完整日志
        class LogMsg{
        public:
            LogMsg(LogLevel level, const std::string& fileName, int line, Logger& logger)
                :_curTime(CurTime())
                ,_level(level)
                ,_pid(::getpid())
                ,_fileName(fileName)
                ,_line(line)
                ,_logger(logger)
                {
                    std::stringstream ssbuf;
                    ssbuf << "[" << _curTime << "] "
                          << "[" << Level2String(_level) << "] "
                          << "[" << _pid << "] "
                          << "[" << _fileName << "] "
                          << "[" << _line << "] - ";
                    _logInfo = ssbuf.str();
                }
            template<typename T>
            LogMsg& operator<<(const T& info){
                std::stringstream ss;
                ss << info;
                _logInfo += ss.str();
                return *this;
            }

            ~LogMsg(){ 
                if(_logger._strategy)
                    _logger._strategy->SyncLog(_logInfo); 
            }
        private:
            std::string _curTime;
            LogLevel _level;
            pid_t _pid;
            std::string _fileName;
            int _line;
            Logger& _logger;
            std::string _logInfo;
        };

        LogMsg operator()(LogLevel level, const std::string& fileName, int line){
            // 返回一个临时对象，利用该对象的析构函数触发写日志
            return LogMsg(level, fileName, line, *this);
        }
    // 开放给LogMsg内部类访问_strategy，也可以私有提供给get方法
    public:
        std::shared_ptr<LogStrategy> _strategy;
    };

    // 全局日志对象
    Logger logger;
// 宏定义简化调用
#define LOG(Level) logger(Level, __FILE__, __LINE__)
#define ENABLE_CONSOLE_LOG() logger.EnableConsoleLog()
#define ENABLE_FILE_LOG() logger.EnableFileLog()
}