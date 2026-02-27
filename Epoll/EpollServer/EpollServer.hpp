#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>


#include "Epoller.hpp"
#include "Connect.hpp"

using namespace EpollerModule;

using connection_t = std::shared_ptr<Connection>;

class EpollServer{
    const static int event_num = 64;
public:
    EpollServer() :_isrunning(false) {}
    void Init() { 
        // 对应成员变量 _epoll是空，直接调用初始化会发生空指针解引用
        // 先创建Epoller对象
        _epoller = std::make_unique<Epoller>();
        _epoller->Init(); 
    }

    // 插入连接：同步哈希表和epoll内核实例
    void InsertConnetion(connection_t &conn){
        auto iter = _connections.find(conn->Sockfd());
        if(iter == _connections.end()){
            // 1. 把连接存入哈希表
            _connections.insert(std::make_pair(conn->Sockfd(), conn));
            // 2. 把连接的fd太你就哀悼epoll实例中，监听指定事件
            _epoller->Ctrl(conn->Sockfd(), conn->GetEvents());
        }
    }

    bool IsConnectionExits(int sockfd){
        return _connections.find(sockfd) != _connections.end();
    }

    void Loop(){
        _isrunning = true;
        while(_isrunning){
            // 1. 等待事件就绪，永久阻塞，直到有事件
            int n = _epoller->Wait(_revs, event_num);
            // 2. 遍历所有就绪事件，分发处理
            for(int i = 0; i < n; i++){
                // 获取就绪fd和事件
                int sockfd = _revs[i].data.fd;
                uint32_t revents = _revs[i].events;

                // 处理异常事件 EPOLLERR/EPOLLHUP
                if((revents & EPOLLERR) || (revents & EPOLLHUP)) 
                    revents = (EPOLLIN | EPOLLOUT); // 转为读写事件

                // 3. 分发读事件，调用Connection的读回调
                if((revents & EPOLLIN) && IsConnectionExits(sockfd)){
                    _connections[sockfd]->CallRecv();
                }

                // 4. 分发写事件
                if((revents & EPOLLOUT) && IsConnectionExits(sockfd)){
                    _connections[sockfd]->CallSend();
                }
            }
        }
        _isrunning = false;
    }
    void Stop() { _isrunning = false; }
    ~EpollServer() {}
private:
    std::unique_ptr<Epoller> _epoller;      // epoll对象，创建epoll模型，这里默认构造是nullptr
    std::unordered_map<int, connection_t> _connections;     // 服务器所有的连接 fd:connection
    bool _isrunning;
    struct epoll_event _revs[event_num];    // 就绪事件数组：存储epoll_wait返回的就绪事件
};