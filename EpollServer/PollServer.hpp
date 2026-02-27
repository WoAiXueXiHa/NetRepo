#pragma once

#include <iostream>
#include <string>
#include <poll.h>
#include "Log.hpp"
#include "Socket.hpp"
#include "Common.hpp"
using namespace SocketModule;
using namespace LogModule;
#define NUM 4096

class PollServer{
public:
    PollServer(int port)
        :_port(port)
        ,_isrunning(false)
        ,_listen_socket(std::make_unique<TcpSocket>())
        {}
    
    void Init(){
        _listen_socket->BuildTcpSocketMethod(_port);

        // 1. 初始化pollfd数组，全部置为无效
        for(int i = 0; i < NUM; i++){
            _fds[i].fd = gdefaultsockfd;
            _fds[i].events = 0;
            _fds[i].revents = 0;
        }

        // 2. 加入监听套接字
        _fds[0].fd = _listen_socket->Fd();
        _fds[0].events = POLLIN;    // 告诉内核，监听他的读事件
    }

    void Accepter(){
        InetAddr client;
        // 此时调用accept绝对不会阻塞，因为poll已经探好路了
        int newfd = _listen_socket->Accepter(&client);
        if(newfd < 0) return;

        std::cout << "获取了一个新连接: " << newfd << " client info: " << client.Addr() << std::endl;

        // 找工位：遍历数组，找一个值为-1的空槽位放入
        int pos = -1;
        for(int j = 0; j < NUM; j++){
            if(_fds[j].fd == gdefaultsockfd){
                pos = j;
                break;
            }
        }

        if(pos == -1){
            LOG(LogLevel::ERROR) << "服务器连接数已经达到上限NUM(" << NUM << ")...";
            ::close(newfd);
        } else {
            // 等级文件描述符，并告诉内核下次请帮我关心他的读事件
            _fds[pos].fd = newfd;
            _fds[pos].events = POLLIN;
        }
    }

    void Recver(int who){
        char buffer[1024];
        ssize_t n = ::recv(_fds[who].fd, buffer, sizeof(buffer) - 1, 0);
        if(n > 0){
            buffer[n] = '\0';
            std::cout << "client# " << buffer << std::endl;

            std::string msg = "echo# ";
            msg += buffer;
            ::send(_fds[who].fd, msg.c_str(), msg.size(), 0);
        } else if(n == 0) {
            // 连接关闭
            LOG(LogLevel::DEBUG) << "客户端退出，sockfd: " << _fds[who].fd;
            ::close(_fds[who].fd);
            _fds[who].fd = gdefaultsockfd;
            _fds[who].events = _fds[who].revents = 0;
        } else {
            // 读取错误
            LOG(LogLevel::DEBUG) << "客户端读取出错，sockfd: " << _fds[who].fd;
            ::close(_fds[who].fd);
            _fds[who].fd = gdefaultsockfd;
            _fds[who].events = _fds[who].revents = 0;
        }
    }
    
    void Dispatcher(){
        for(int i = 0; i < NUM; i++){
            // 1. 过滤掉还没被使用的空槽位
            if(_fds[i].fd == gdefaultsockfd) continue;

            // 2. 检查具体的就绪事件，是否是我们关心的读事件发生？
            if(_fds[i].revents & POLLIN){
                // 3. 核心分流，是新链接到来，还是老链接发数据了？
                if(_fds[i].fd == _listen_socket->Fd())
                    Accepter();
                else
                    Recver(i);
            }
        }   
    }
    void Loop(){
        _isrunning = true;
        
        while(_isrunning){
            // 这里不需要遍历辅助数组了

            // poll系统调用：传入数组首地址、数组大小、超时时间（-1代表阻塞）
            int n = ::poll(_fds, NUM, -1);
            switch(n){
                case -1:
                    perror("poll");
                    break;
                case 0:
                    std::cout << "timeout...\n";
                    break;
                default:
                    // 有事件就绪了
                    // 内核通知用户：你关心的fd里，有事件发生了
                    std::cout << "有事件就绪了！\n";
                    Dispatcher();
                    break;
            }
        }
        _isrunning = false;
    }

    ~PollServer(){}
private:
    struct pollfd _fds[NUM];
    uint16_t _port;
    std::unique_ptr<Socket> _listen_socket;
    bool _isrunning;
};