#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <pthread.h>

namespace ThreadModule{
    // 定义线程要执行的任务类型
    // void(std::string name):线程执行时，可以把线程的名字传给它，方便打印日志
    using func_t = std::function<void(std::string)>;

    class Thread{
    public:
        // 构造：只初始化，不启动
        Thread(func_t func, const std::string& name = "Thread-Base")
            :_func(func)
            ,_name(name)
            ,_is_running(false)
            {}

        // 启动线程
        void Start(){
            _is_running = true;
            // 传入Routine作为入口函数
            // 传入this指针作为参数，让静态函数找到对象
            int n = ::pthread_create(&_tid, nullptr, Routine, this);
            if(0 != n){
                std::cerr << "Thread create failed!\n";
                _is_running = false;
            }
        }

        // 等待线程结束
        void Join(){
            if(_is_running){
                ::pthread_join(_tid, nullptr);
                _is_running = false;
            }
        }

        std::string Name() const { return _name; }

        ~Thread(){
            // 析构时不要Join，由外部管理生命周期
        }
    private:
        // 核心：静态成员函数做跳板
        static void* Routine(void* args){
            // 1. 恢复this指针
            Thread* self = static_cast<Thread*>(args);
            // 2. 调用真正的业务
            self->_func(self->_name);
            return nullptr;
        }
    private:
        pthread_t _tid;
        std::string _name;
        func_t _func;           // 线程要执行的任务
        bool _is_running;
    };
}