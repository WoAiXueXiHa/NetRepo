#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <cerrno>  
#include <cstring> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "Common.hpp"
#include "Log.hpp"
#include "InetAddr.hpp"

namespace SocketModule{
    // 前置声明
    class Socket;
    // 使用智能指针管理
    using SockPtr = std::shared_ptr<Socket>;

    // 抽象基类
    class Socket{
    public:
        virtual ~Socket() = default;

        // 纯虚函数接口：派生类必须实现干活的步骤
        // 1. 创建套接字
        virtual void SocketOrDie() = 0;
        // 2. 设置端口复用
        virtual void SetSocketOpt() = 0;
        // 3. 绑定端口
        virtual bool BindOrDie(uint16_t port) = 0;
        // 4. 监听
        virtual bool ListenOrDie() = 0;
        // 5. 接收连接，返回一个指向新连接的智能指针
        virtual SockPtr Accepter(InetAddr* client) = 0;

        // 基础IO
        virtual int Recv(std::string* out) = 0;
        virtual int Send(const std::string& in) = 0;
        virtual void Close() = 0;
        virtual int Fd() = 0;   // 获取原生fd

        // 模板方法模式
        // 把流程定死，派生类只负责实现细节
        // 任何TCP服务器启动必须遵守这个流程
        void BuildTcpSocketMethod(int port){
            SocketOrDie();
            SetSocketOpt();
            BindOrDie(port);
            ListenOrDie();
        }
    };

    class TcpSocket : public Socket{
    public:
        // 默认构造
        TcpSocket() :_sockfd(gdefaultsockfd) {}
        // 用现有的fd创建对象
        TcpSocket(int sockfd) :_sockfd(sockfd) {}
        virtual ~TcpSocket() { Close();}

        // 1. 创建套接字
        virtual void SocketOrDie() override{
            _sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
            if(_sockfd < 0){
                LOG(ERROR) << "socket error: " << errno << std::endl;
                exit(SOCKET_ERR);
            }
            LOG(DEBUG) << "socket success!\n";
        }

        // 2. 设置端口复用
        virtual void SetSocketOpt() override{
            int opt = 1;
            ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        }

        // 3. 绑定端口
        virtual bool BindOrDie(uint16_t port) override{
            if(_sockfd < 0) return false;
            InetAddr addr(port);
            int n = ::bind(_sockfd, addr.NetAddr(), addr.NetAddrLen());
            if(n < 0){
                LOG(ERROR) << "bind error: " << errno << std::endl;
                exit(BIND_ERR);
            }
            LOG(DEBUG) << "bind success!\n";
            return true;
        }
        // 4. 监听
        virtual bool ListenOrDie() override{
            if(_sockfd < 0) return false;
            int n = ::listen(_sockfd, gbacklog);
            if(n < 0){
                LOG(ERROR) << "listen error: " << errno << std::endl;
                exit(LISTEN_ERR);
            }
            LOG(DEBUG) << "listen success!\n";
            return true;
        }
        // 5. 接收连接，返回一个指向新连接的智能指针
        virtual SockPtr Accepter(InetAddr* client) override{
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            
            // accept 从全连接队列里拿出一个已经建立好的连接
            // 返回一个新的fd，专门用于服务客户端
            // 原来的_sockfd继续监听
            int newsockfd = ::accept(_sockfd, Conv(&peer), &len);
            if(newsockfd < 0){
                return nullptr;     //失败就下一个
            }

            // 把对方的IP/Port信息填回去，给上层打印日志
            if(client) client->SetAddr(peer,len);
            LOG(INFO) << "create a new connection success! peer info: " << client->Addr() << std::endl;

            // 返回一个新的TcpSocket对象管理这个新连接
            return std::make_shared<TcpSocket>(newsockfd);
        }

        // 封装 recv
        virtual int Recv(std::string* out) override{
            char buffer[4096];
            // 简单粗暴读取
            ssize_t n = ::recv(_sockfd, buffer, sizeof(buffer) - 1, 0);
            if(n > 0){
                buffer[n] = 0;
                *out += buffer;
            }
            return n;
        }

        // 封装send
        virtual int Send(const std::string& in) override{
            return ::send(_sockfd, in.c_str(), in.size(), 0);
        }

        void Close() override{
            if(_sockfd >= 0){
                ::close(_sockfd);
                _sockfd = gdefaultsockfd;
            }
        }

        virtual int Fd() override { return _sockfd; }
    private:
        int _sockfd;
    };
}
