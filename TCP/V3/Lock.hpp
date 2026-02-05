#pragma once
#include <pthread.h>

// 把RAII锁和条件变量封装到一个文件里
// 条件变量唤醒需要用到锁

namespace LockModule{
    class Mutex{
    public:
        Mutex(const Mutex&) = delete;
        const Mutex& operator=(const Mutex&) = delete;
        Mutex() { ::pthread_mutex_init(&_lock, nullptr); }
        ~Mutex() { ::pthread_mutex_destroy(&_lock); }
        void Lock() { ::pthread_mutex_lock(&_lock); }
        void UnLock() { ::pthread_mutex_unlock(&_lock); }
        pthread_mutex_t* GetLock() { return &_lock; }
    private:
        pthread_mutex_t _lock;
    };

    class LockGuard{
    public:
        LockGuard(Mutex& mutex)
            :_mutex(mutex)
            {
                _mutex.Lock(); 
            }

        ~LockGuard() { _mutex.UnLock(); }
    private:
        Mutex& _mutex;
    };

    // 条件变量封装，用于线程间“通知-等待”机制
    // 场景：生产者-消费者模型（线程池中worker没任务时要等待）
    class Cond{
    public:
        Cond() { ::pthread_cond_init(&_cond, nullptr); }
        ~Cond() { ::pthread_cond_destroy(&_cond); }
        // 等待：释放锁->挂起线程->等待被唤醒->重新竞争锁
        // 必须配合Mutex使用，防止竞态条件
        void Wait(Mutex& mutex){ ::pthread_cond_wait(&_cond, mutex.GetLock()); }
        // 唤醒
        void NotifyOne() { ::pthread_cond_signal(&_cond); }
        void NotifyAll() { ::pthread_cond_broadcast(&_cond); }
    private:
        pthread_cond_t _cond;
    };
}