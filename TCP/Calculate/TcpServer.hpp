#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <functional>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "InetAddr.hpp"
#include "ThreadPool.hpp"

using namespace ThreadPoolModule;

const static int gport = 8080;
const static int gbacklog = 5;

// 业务层需要实现的接口形式
using Handler = std::function<void(int,InetAddr)>;

class TcpServer{
public:
    TcpServer(int port = gport)
        :_port(port)
        ,_listensock(-1)
        ,_is_running(false)
        {
            // 1. 创建线程池
            _threadpool = new ThreadPool<std::function<void()>>(5);

            // 2. 忽略SIGPIPE信号
            // 防止客户端断开连接时，服务器写入触发信号导致进程崩溃
            signal(SIGPIPE, SIG_IGN);
        }

    void InitServer(){
        // 1. 创建套接字
        _listensock = ::socket(AF_INET, SOCK_STREAM, 0);
        if(_listensock < 0){
            std::cerr << "[FATAL] Socket create failed!\n";
            exit(1);
        }

        // 2. 端口复用
        // 允许服务器重启后立刻绑定原来的端口
        int opt = 1;
        ::setsockopt(_listensock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEADDR, &opt, sizeof(opt));

        // 3. 绑定
        InetAddr local(_port);
        if(::bind(_listensock, local.NetAddr(), local.NetAddrLen()) < 0){
            std::cerr << "[FATAL] Bind failed!\n";
            exit(2);
        }

        // 4. 监听
        if(::listen(_listensock, gbacklog) < 0){
            std::cerr << "[FATAL] Listen failed!\n";
            exit(3);
        }

        std::cout << "[INFO] Server init succss, Listening on port: " << _port << "\n";
    }

    // 核心循环： 负责accept连接并分发给线程池
    void Loop(Handler handler){
        _is_running = true;

        // 启动线程池
        _threadpool->Start();
        std::cout << "[INFO] ThreadPool started, waiting for connection...\n";

        while(_is_running){
            // 1. accept 拉客
            struct sockaddr_in peer;
            socklen_t peerlen = sizeof(peer);

            // 阻塞等待新连接
            int sockfd = ::accept(_listensock, (struct sockaddr*)&peer, &peerlen);
            if(sockfd < 0){
                std::cerr << "[WARNING] Accept failed, continue...\n";
                continue;
            }

            // 2. 获取客户端信息
            InetAddr client(peer);
            std::cout << "[INFO] New connection: " << client.Ip() << ":" << client.Port() << "\n";

            // 3. 构建任务
            // 使用lambda捕获sockfd和client信息
            auto task = [handler, sockfd, client](){
                // 执行具体业务
                handler(sockfd, client);

                // 业务结束关闭
                ::close(sockfd);
                std::cout << "[INFO] Connection closed: " << client.Ip() << ":" << client.Port() << "\n";
            };

            // 4. 扔进线程池队列
            _threadpool->Enqueue(task);
        }
    }

    ~TcpServer() {
        if (_listensock >= 0) ::close(_listensock);
        if (_threadpool) {
            _threadpool->Stop(); // 优雅退出
            delete _threadpool;
        }
    }
private:
    int _port;
    int _listensock;
    bool _is_running;

    ThreadPool<std::function<void()>>* _threadpool;
};