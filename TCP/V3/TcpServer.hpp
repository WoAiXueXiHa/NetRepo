#pragma once

#include <iostream>
#include <functional>
#include <unistd.h>
#include <signal.h>
#include "InetAddr.hpp"
#include "Log.hpp"
#include "ThreadPool.hpp"

using namespace ThreadPoolModule;
using namespace LogModule;

// 定义任务类型
using Task = std::function<void(std::string)>;
// 定义业务返回逻辑
// 这是TcpServer给上层业务用的，不关心线程名，只关心Socket和地址
using Handler = std::function<void(int, InetAddr)>;

class TcpServer{
public:
    TcpServer(uint16_t port = gport)
        :_port(gport)
        ,_listensock(gfd)
        ,_is_running(false)
        ,_threadpool(new ThreadPool<Task>(5))
        {
            // 忽略SIGPIPE信号
            // 如果不忽略，当客户端断开连接时，服务器还在往里写数据
            // os会发信号把服务器杀掉
            signal(SIGPIPE, SIG_IGN);
        }
    
    void InitServer(){
        // 1. 创建socket
        _listensock = ::socket(AF_INET, SOCK_STREAM, 0);
        if(_listensock < 0){
            LOG(FATAL) << "Socket create failed";
            exit(SOCKET_ERR);
        }
        LOG(INFO) << "Socket create success!";

        // 2. 端口复用，编码TIME_WAIT导致绑定失败
        int opt = 1;
        ::setsockopt(_listensock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // 3. 绑定 bind
        InetAddr local(_port);
        // 在地址转换文件里，需要一个返回struct sockaddr* 类型的原生指针，是为了满足接口参数需求
        int n = ::bind(_listensock, local.NetAddr(), local.NetAddrLen());
        if(n < 0){
            LOG(FATAL) << "Bind failed!";
            exit(BIND_ERR);
        }
        LOG(INFO) << "Bind success!";

        // 4. 监听
        n = ::listen(_listensock, BACKLOG);
        if(n < 0){
            LOG(FATAL) << "Listen failed!";
            exit(LISTEN_ERR);
        }
        LOG(INFO) << "Server init success, listening on port: " << _port;
    }

    void Loop(Handler handler){
        _is_running = true;
        _threadpool->Start();
        LOG(INFO) << "TcpServer Loop starting...";

        while(_is_running){
            struct sockaddr_in peer;
            socklen_t peerlen = sizeof(peer);

            // 5. 阻塞等待连接 accept
            int sockfd = ::accept(_listensock, Conv(&peer), &peerlen);
            if(sockfd < 0){
                LOG(WARNING) << "Accept failed, continue...";
                continue;
            }
            InetAddr client(peer);
            LOG(INFO) << "New connection: " << client.Ip() << ", fd: " << sockfd; 

            // 6. 构建任务
            // 把handler、sockfd、client绑定到一个lambda里
            // lambda必须接收std::string参数，ThreadPool会传name进来
            Task task = [handler, sockfd, client](std::string thread_name){
                LOG(INFO) << "[" << thread_name << "] is handing client: " << client.Ip();

                // 执行真正的业务
                handler(sockfd, client);

                // 业务处理完，关闭连接
                ::close(sockfd);
                LOG(INFO) << "[" << thread_name << "] close client fd: " << sockfd;
            };

            // 7. 任务入队
            _threadpool->Equeue(std::move(task));
        }
    }
    
    ~TcpServer(){
        if(_listensock >= 0) ::close(_listensock);
        // 释放线程资源
        _threadpool->Stop();
        _threadpool->Wait();
        delete _threadpool;
    }
private:
    uint16_t _port;
    int _listensock;
    bool _is_running;
    ThreadPool<Task>* _threadpool;
};