#pragma once

#include <iostream>
#include <string>
#include <functional>

#include "InetAddr.hpp"

class EpollServer;

using func_t = std::function<void()>;

class Connection{
public:
    Connection(int sockfd)
        :_sockfd(sockfd)
        ,_events(0)
        ,_owner(nullptr)    
    {}

    // 初始化回调事件
    void InitCB(func_t recver, func_t sender, func_t excepter){
        _recver = recver;
        _sender = sender;
        _excepter = excepter;
    }

    // 设置客户端地址
    void SetClientInfo(const InetAddr &peer_addr){
        _peer_addr = peer_addr;
    }
    int Sockfd() { return _sockfd; }
    void SetEvents(uint32_t events) { _events = events; }
    uint32_t GetEvents() { return _events; }

    // 执行回调函数
    void CallRecv() {
        if(_recver != nullptr) _recver();
    }
    void CallSend() {
        if(_sender != nullptr) _sender();
    }
    void CallExcept(){
        if(_excepter != nullptr) _excepter();
    }
    ~Connection(){}
private:
    int _sockfd;    // 连接对应的文件描述符
    std::string _inbuffer;  // 读缓冲区，存储从fd读取的数据
    std::string _outbuffer; // 写缓冲区，存储哟啊写入fd的数据
    InetAddr _peer_addr;    // 客户端地址
    
    // 回调函数
    func_t _recver;         // 读事件
    func_t _sender;         // 写事件
    func_t _excepter;       // 异常事件

    EpollServer *_owner;    // 所属EpollServer的实例，属于什么服务器就回指向哪个服务器

    uint32_t _events;       // 该连接关心的epoll事件
};