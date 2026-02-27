#pragma once

#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/epoll.h>
#include "Common.hpp"
#include "Log.hpp"

namespace EpollerModule{
    using namespace LogModule;
    class Epoller{
    public:
        Epoller()
            :_epfd(-1)
        {}
        
        void Init(){
            int n = ::epoll_create(256);
            if(n < 0){
                LOG(LogLevel::ERROR) << "epoll_create error!";
                exit(EPOLL_CREATE_ERR);
            }
            // 构造时_epfd为-1就一直没管！
            _epfd = n;
            LOG(LogLevel::INFO) << "epoll create success, epfd: " << _epfd;
        }

        int Wait(struct epoll_event revs[], int num){
            int timeout = -1;
            int n = ::epoll_wait(_epfd, revs, num, timeout);
            if(n < 0){
                LOG(LogLevel::WARNING) << "epoll_wait error!" << errno << ", des: " << strerror(errno);
            }
            return n;
        }

        void Ctrl(int sockfd, uint32_t events){
            struct epoll_event ev;
            // 设置监听事件
            ev.events = events;
            // 关联当前的sockfd
            ev.data.fd = sockfd;
            int n = ::epoll_ctl(_epfd, EPOLL_CTL_ADD, sockfd, &ev);
            if(n < 0){
                LOG(LogLevel::WARNING) << "epoll_ctl error!";
            }
        }

        ~Epoller() {
            ::close(_epfd);
        }
    private:
        int _epfd;
    };
}