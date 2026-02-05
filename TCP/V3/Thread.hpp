#pragma once
#include <iostream>
#include <string>
#include <mutex>
#include <functional>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

namespace ThreadModule{
    // 定义任务函数类型，线程入口逻辑
    using func_t = std::function<void(std::string name)>;
    static int thread_num = 1;

    enum STATUS{ NEW, RUNNING, STOP };

    class Thread{
    private:
        // 线程的入口函数
        static void* Routine(void* arg){
            // 恢复this指针本体
            Thread* t = static_cast<Thread* >(arg);
            t->_status = RUNNING;
            t->_func(t->getName());
            return nullptr;
        }

        void EnableDetach() { _joinable = false; }
    public:
        Thread(func_t func)
            :_func(func)
            ,_status(NEW)
            ,_joinable(true)
            {
                _name = "Thread-" + std::to_string(thread_num++);
                _pid = ::getpid();
            }
        
            bool Start(){
                if(_status != RUNNING){
                    int n = ::pthread_create(&_tid, nullptr, Routine, this);
                    if(0 != n) return false;
                }
                return true;
            }

            bool Stop(){
                if(_status == RUNNING){
                    int n = ::pthread_cancel(_tid);
                    if(0 != n) return false;
                    _status = STOP;
                    return true;
                }
                return false;
            }

            bool Join(){
                if(_joinable){
                    int n = ::pthread_join(_tid, nullptr);
                    if(0 != n) return false;
                    _status = STOP;
                    return true;
                }
                return false;
            }

            void Detach(){
                EnableDetach();
                pthread_detach(_tid);
            }

            bool IsJoinable() { return _joinable; }
            std::string getName() const { return _name; }
            ~Thread(){}
    private:
        std::string _name;
        pthread_t _tid;
        pid_t _pid;
        bool _joinable;
        func_t _func;
        STATUS _status;
    };
}