#pragma once

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <memory>
#include "Log.hpp"
#include "Lock.hpp"
#include "Thread.hpp"
#include "Log.hpp"

namespace ThreadPoolModule{
    using namespace LogModule;
    using namespace ThreadModule;
    using namespace LockModule;

    // 线程指针类型
    using thread_t = std::shared_ptr<Thread>;
    const static int defaultNum = 5;

    template <typename T>
    class ThreadPool{
    private:
        bool IsEmpty() { return _taskq.empty(); }
        // 消费者
        // 这个函数会多个线程同时执行
        void HandlerTask(std::string name){
            LOG(INFO) << "线程: " << name << ", 进入HandlerTask的逻辑";
            while(1){
                // 1. 拿任务,加锁
                T t;
                {
                    LockGuard lockGuard(_lockq);
                    while(IsEmpty() && _is_running){
                        _wait_num++;
                        _cond.Wait(_lockq);
                        _wait_num--;
                    }

                    // 如果队列空了且线程池被标记为停止运行，break跳出循环
                    if(IsEmpty() && !_is_running) break;

                    //  取任务
                    t = _taskq.front();
                    _taskq.pop();
                }

                // 2. 处理任务，放在临界区外面，否则串行执行，失去多线程并发的意义
                t(name);
            }
            LOG(INFO) << "线程: " << name << " 退出";
        }
    public:
        ThreadPool(int num = defaultNum)
            :_num(num)
            ,_wait_num(0)
            ,_is_running(false)
            {
                for(int i = 0; i < _num; i++){
                    // std::bind 把this指针和占位符传进去，适配Thread接口
                    _threads.push_back(std::make_shared<Thread>(std::bind(&ThreadPool::HandlerTask, this, std::placeholders::_1)));
                    LOG(INFO) << "构建线程" << _threads.back() ->getName() << "对象...成功";
                }
            }

        // 生产者，入队
        void Equeue(T&& in){
            LockGuard lockGuard(_lockq);
            if(!_is_running) return;
            _taskq.push(std::move(in));

            if(_wait_num > 0) _cond.NotifyOne();
        }

        void Start(){
            if(_is_running) return;
            _is_running = true;
            for(auto& thread_ptr : _threads){
                LOG(INFO) << "启动线程" << thread_ptr->getName() << " ... 成功";
                thread_ptr->Start();
            }
        }

        void Wait(){
            for(auto& thread_ptr : _threads){
                thread_ptr->Join();
                LOG(INFO) << "回收线程" << thread_ptr->getName() << " ... 成功";
            }
        }

        void Stop(){
            LockGuard lockGuard(_lockq);
            if(_is_running){
                _is_running = false;
                if(_wait_num > 0) _cond.NotifyAll();
            }
        }

        ~ThreadPool(){}
    private:
        std::vector<thread_t> _threads;         // 线程对象容器
        int _num;
        int _wait_num;                          // 等待的线程数
        std::queue<T> _taskq;                   // 任务队列，临界资源

        Mutex _lockq;                           // 保护任务队列的锁
        Cond _cond;                             // 生产消费同步的条件变量

        bool _is_running;
    };
}