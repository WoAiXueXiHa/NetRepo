#pragma once
#include <iostream>
#include <pthread.h>

namespace LockModule{
    // 1. 封装原生锁对象
    class Mutex{
    public:
        // ==========================================================
        // 重要！！！禁止拷贝！！！
        // 锁是系统资源，不允许被复制，如果复制了，两个对象管理同一个锁
        // 析构时会destroy两次，导致程序崩溃
        Mutex(const Mutex&) = delete;
        const Mutex& operator=(const Mutex&) = delete; 
        // ==========================================================

        Mutex(){
            int n = ::pthread_mutex_init(&_lock, nullptr);
            if(0 != n){
                std::cerr << "Mutex init failed!\n";
            }
        }

        ~Mutex() { ::pthread_mutex_destroy(&_lock); }
        void Lock() { ::pthread_mutex_lock(&_lock); }
        void Unlock(){ ::pthread_mutex_unlock(&_lock); }

        // 获取原生锁指针，条件变量会用到
        pthread_mutex_t* GetLock(){ return &_lock; }
    private:
        pthread_mutex_t _lock;
    };

    // 2. 封装RAII风格的锁
    class LockGuard{
    public:
        // 构造时自动加锁
        // 关键：必须传引用！！！！
        // 传值会发生拷贝，上面禁止拷贝了，那么编译器会报错
        // 即使允许拷贝，锁住的是副本，原来的锁根本没动，没起到保护作用！
        LockGuard(Mutex& mutex) : _mutex(mutex) { _mutex.Lock(); }

        // 析构时自动解锁
        ~LockGuard() { _mutex.Unlock(); }
    private:
        Mutex& _mutex;          // 必须是引用类型，直接修改本身而不是副本
    };

    class Cond{
    public:
        Cond(){
            int n = ::pthread_cond_init(&_cond, nullptr);
            if(0 != n){
                std::cerr << "Cond init failed!\n";
            }
        }

        ~Cond() { ::pthread_cond_destroy(&_cond); }

        // 等待
        // 等待时，必须把锁传进去！
        // why？pthread_cond_wait会在沉睡前自动释放锁
        // 醒来后重新竞争锁，如果不传锁，无法原子操作
        void Wait(Mutex& mutex){
            ::pthread_cond_wait(&_cond, mutex.GetLock());
        }

        // 唤醒一个线程
        void Notify(){ :: pthread_cond_signal(&_cond); }
        // 唤醒所有线程
        void NotifyAll() { ::pthread_cond_broadcast(&_cond); }

    private:
        pthread_cond_t _cond;
    };
}