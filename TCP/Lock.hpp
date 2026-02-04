#pragma once 
#include <thread>

namespace LockModule{
    // 封装原生锁
    class Mutex{
    public:
        // 禁止拷贝
        // 锁是系统资源，如果拷贝，两个对象指向同一个底层锁，析构会析构两次
        Mutex(const Mutex&) = delete;
        const Mutex& operator=(const Mutex&) = delete;

        Mutex() { ::pthread_mutex_init(&_lock, nullptr); }
        ~Mutex() { ::pthread_mutex_destroy(&_lock); }
        void Lock() { ::pthread_mutex_lock(&_lock); }
        void Unlock() { ::pthread_mutex_unlock(&_lock); }
        // 向外部暴露原生指针,配合pthread_cond_wait使用
        pthread_mutex_t* GetLock() { return &_lock; }
    private:
        pthread_mutex_t _lock;
    };

    // 封装RAII机制锁
    class LockGuard{
    public:
        LockGuard(Mutex& mutex) :_mutex(mutex) { _mutex.Lock(); }
        ~LockGuard() { _mutex.Unlock(); }
    private:
        Mutex& _mutex;
    };
}