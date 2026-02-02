#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>
#include <unistd.h>
#include <time.h>
#include <cstring>
#include <cerrno>
#include "Lock.hpp"

namespace LogModule{
    using namespace LockModule;

    // ------------------------------------------------
    // 工具函数：获取当前时间
    // YYYY-MM-DD HH:MM:SS
    // localtime_r 保证线程安全
    // ------------------------------------------------
    std::string CurTime(){
        time_t time_cur = ::time(nullptr);
        struct tm cur;
        // _r版本可重入，保存在用户提供的struct tm cur中
        localtime_r(&time_cur, &cur);
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "%4d-%02d-%02d %02d:%02d:%02d",
                 cur.tm_year + 1900,
                 cur.tm_mon + 1,
                 cur.tm_mday,
                 cur.tm_hour,
                 cur.tm_min,
                 cur.tm_sec
                );
        return buffer;
    }

    const std::string defaultLogPath = "./Log/";
    const std::string defaultLogName = "server_log.txt";

    // 日志等级
    enum LogLevel{
        DEBUG = 1,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    // 枚举转字符串
    std::string Level2String(LogLevel level){
        switch(level){
            case DEBUG : return "DEBUG";
            case INFO : return "INFO";
            case WARNING : return "WARNING";
            case ERROR : return "ERROR";
            case FATAL : return "FATAL";
            default : return "None";
        }
    }

    // ------------------------------------------------
    // 策略模式：定义日志策略基类 LogStrategy
    // 定义刷盘接口策略：派生类必须实现SyncLog方法
    // ------------------------------------------------
    class LogStrategy{
    public:
        virtual ~LogStrategy() = default;
        virtual void SyncLog(const std::string& msg) = 0;
    };

    // 策略1：控制台策略
    class ConsoleLogStrategy : public LogStrategy{
    public:
        void SyncLog(const std::string& msg) override{
            LockGuard lockGuard(_lock); //控制台也是临界资源，防止输出乱序
            std::cout << msg << std::endl;
        }
    private:
        Mutex _lock;
    };

    // 策略2：文件策略
    // 成员变量 _out 常驻内存，避免每次写日志都open/close
    class FileLogStrategy : public LogStrategy{
    public:
        FileLogStrategy(const std::string path = defaultLogPath, 
                        const std::string name = defaultLogName)
            :_logPath(path)
            ,_logName(name)
            {
                // 1. 如果不存在，自动创建日志目录
                if(!std::filesystem::exists(_logPath)){
                    try{
                        std::filesystem::create_directories(_logPath);
                    } catch(std::filesystem::filesystem_error& e){
                        std::cerr << "Create Log Dir Error: " << e.what() << std::endl;
                        return;
                    }
                }

                // 2. 打开文件，追加模式
                std::string log = _logPath + _logName;
                _out.open(log, std::ios::app);
                if(!_out.is_open()){
                    std::cerr << "Log file open error!" << std::endl;
                }
            }

        ~FileLogStrategy() { if(_out.is_open()) _out.close(); }

        void SyncLog(const std::string& msg) override{
            LockGuard lockGuard(_lock);
            if(_out.is_open()){
                _out << msg << "\n";
            }
        }
    private:
        Mutex _lock;
        std::string _logPath;
        std::string _logName;
        std::ofstream _out;
    };

    // ------------------------------------------------
    // 核心管理类：日志类Logger
    // 组装日志信息，调用Strategy输出
    // ------------------------------------------------
    class Logger{
    public:
        // 默认输出到控制台
        Logger() { _strategy = std::make_shared<ConsoleLogStrategy>(); }
        void EnableConsoleLog() { _strategy = std::make_shared<ConsoleLogStrategy>(); }
        void EnableFileLog() { _strategy = std::make_shared<FileLogStrategy>(); }

        // ------------------------------------------------
        // 内部类：LogMsg
        // 承载一条日志的声明周期
        // LOG(INFO) << "xxx" 回创建一个LogMsg的临时对象
        // 对象析构时，回自动把拼接好的字符串刷入Strategy
        // ------------------------------------------------
        class LogMsg{
        public:
            LogMsg(LogLevel level, const std::string& fileName, int line, Logger& logger)
                :_logger(logger)
                {
                    std::stringstream ssbuf;
                    ssbuf << "[" << CurTime() << "] "
                          << "[" << Level2String(level) << "] "
                          << "[" << fileName << ":" << line << "] - ";
                    _logInfo = ssbuf.str();
                }

            // 支持链式调用
            template <typename T>
            LogMsg& operator<<(const T& info){
                std::stringstream ss;
                ss << info;
                _logInfo += ss.str();
                return *this;   // 返回自身引用保证链式调用
            }

            // 析构时触发写入
            ~LogMsg() { 
                if(_logger._strategy)
                    _logger._strategy->SyncLog(_logInfo);
            }            
        private:
            Logger& _logger;
            std::string _logInfo;
        };

        // 仿函数：让Logger对象像函数一样被调用
        LogMsg operator()(LogLevel level, const std::string& fileName, int line){
            return LogMsg(level, fileName, line, *this);
        }
    public:
        // 暴露给内部类，也可以使用get方法
        std::shared_ptr<LogStrategy> _strategy;
    };

    Logger logger;

    #define LOG(Level) logger(Level, __FILE__, __LINE__)
    #define ENABLE_CONSOLE_LOG() logger.EnableConsoleLog()
    #define ENABLE_FILE_LOG() logger.EnableFileLog()
}