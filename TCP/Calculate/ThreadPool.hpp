#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include "Thread.hpp"
#include "Lock.hpp"

using namespace ThreadModule;
using namespace LockModule;

namespace ThreadPoolModule{
    // T是任务类型，未来传给它 std::function<void()> 或者 Task对象
    template <typename T>
    class ThreadPool{
    private:
        // 消费者接口：线程工人
        void HandlerTask(std::string name){
            std::cout << name << " start working...\n";
            while(1){
                T task;
                {
                    // 定义临界区
                    LockGuard lock(_mutex);
                
                    // 为什么用while不用if？
                    // 防止虚假唤醒->有时候线程可能莫名其妙醒来（OS原因）
                    // 或者被唤醒了但是任务被别人抢走了
                    // 必须循环检查队列是否真的有货
                    while(_task_queue.empty() && _is_running){
                        _cond.Wait(_mutex);     // 没货就睡觉
                    }

                    if(!_is_running && _task_queue.empty()) break;

                    // 取任务
                    task = _task_queue.front();
                    _task_queue.pop();
                }
                // 出了括号，锁自动释放
                // 任务处理一定在锁外面！！！
                // 否则串行执行，多线程无意义

                // 执行任务
                task();
            }
        }
    public:
        ThreadPool(int num = 5)
            :_thread_num(num)
            ,_is_running(false)
            {
                for(int i = 0; i < _thread_num; i++){
                    // Thread需要一个 void(std::string)类型的安徽念书
                    // HandlerTask是成员函数，有一个this指针参数
                    // 用bind把this绑定死，编程一个可以直接调用的函数对象
                    func_t func = std::bind(&ThreadPool::HandlerTask, this, std::placeholders::_1);

                    _threads.push_back(new Thread(func, "Thread-" + std::to_string(i)));
                }
            }
        
        // 启动线程
        void Start(){
            _is_running = true;
            for(auto t : _threads){
                t->Start();
            }
        }

        // 停止线程池
        void Stop(){
            if(_is_running){
                _is_running = false;
                _cond.NotifyAll();
                for(auto t : _threads){
                    t->Join();
                }
            }
        }

        // 生产者接口：塞任务
        void Enqueue(const T& task){
            LockGuard lock(_mutex);
            if(_is_running){
                _task_queue.push(task);
                _cond.Notify();
            }
        }
        
        ~ThreadPool(){
            Stop();
            for(auto t : _threads){
                delete t;
            }
        }
    private:
        int _thread_num;
        bool _is_running;
        std::vector<Thread*> _threads;          // 线程容器
        std::queue<T> _task_queue;              // 任务队列
        Mutex _mutex;                           // 保护队列的锁               // 
        Cond _cond;                             // 协调生产消费的条件变量
    };
}