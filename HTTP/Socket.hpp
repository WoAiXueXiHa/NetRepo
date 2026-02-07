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
#include "InetAddr.hpp"

namespace SocketModule{
    // 未来返回的都是智能指针，防止内存泄漏，无需手动delete
    class Socket;
    using SockPtr = std::shared_ptr<Socket>;

    // ================== 基类 ========================
    class Socket{
    public:
        virtual ~Socket() = default;

        // 纯虚函数：派生类必须实现这些功能
        virtual void SocketOrDie() = 0;                         // 1. 创建
        virtual void SetSocketOpt() = 0;                        // 2. 设置选项
        virtual bool BindOrDie(uint16_t port) = 0;              // 3. 绑定
        virtual bool ListenOrDie() = 0;                         // 4. 监听
        virtual SockPtr Accept(InetAddr* client) = 0;           // 5. 获取连接
        virtual void Close() = 0;
        virtual int Recv(std::string* out) = 0;
        virtual int Send(const std::string& in) = 0;

        // 封装TCP服务器初始化的标准流程，不允许派生类修改顺序
        void BuildTcpSocketRule(uint16_t port){
            SocketOrDie();
            SetSocketOpt();
            BindOrDie(port);
            ListenOrDie();
        }
    };

    // ================== tcp实现类 =====================
    class TcpSocket : public Socket{
    public:
        TcpSocket() : _sockfd(gdefaultsockfd) {}
        TcpSocket(int sockfd) : _sockfd(sockfd) {} // 给accept用
        virtual ~TcpSocket() { Close(); }

        // 1. 创建套接字
        virtual void SocketOrDie() override{
            _sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
            if(_sockfd < 0){
                std::cerr << "socket create error" << std::endl;
                exit(1);
            }
        }           
        
        // 2. 设置端口复用
        virtual void SetSocketOpt() override{
            int opt = 1;
            // 解决TIME_WAIT导致的绑定错误
            // 服务器重启时，如果端口还处于TIME_WAIT状态
            // SO_REUSEADDR允许立即强行绑定该端口
            ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        }                     

        // 3. 绑定端口
        virtual bool BindOrDie(uint16_t port) override{
            InetAddr addr(port);
            int n = ::bind(_sockfd, addr.NetAddr(), addr.NetAddrLen());
            if (n < 0) {
                std::cerr << "bind error: " << strerror(errno) 
                          << " (errno: " << errno << ")" << std::endl;
                exit(2);
            }
            return true;
        }              

        // 4. 监听
        virtual bool ListenOrDie() override{
            int n = ::listen(_sockfd, gbacklog);
            if (n < 0) {
                std::cerr << "listen error" << std::endl;
                exit(3);
            }
            return true;
        }        
        
        // 5. 获取连接，我需要知道连接的是谁
        virtual SockPtr Accept(InetAddr* client) override{
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            // accept是阻塞的，从全连接队列里拿出一个已经建立好的连接
            // _sockfd是监听用的，new_sockfd是专门服务这个客户端的
            int new_sockfd = ::accept(_sockfd, Conv(&peer), &len);
            if(new_sockfd < 0) return nullptr;

            // 复现需求，回填客户端信息
            if(client) client->SetAddr(peer);

            // 返回管理新文件描述符的智能指针
            return std::make_shared<TcpSocket>(new_sockfd);
        }         

        virtual void Close() override{
            if(_sockfd != gdefaultsockfd){
                ::close(_sockfd);
                _sockfd = gdefaultsockfd;
            }
        }

        // 读数据：追加到string
        virtual int Recv(std::string* out) override{
            char buffer[4096];
            ssize_t n = ::recv(_sockfd, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = 0;
                *out += buffer;
            }
            return n;
        }

        // 发数据
        virtual int Send(const std::string& in) override{
            return ::send(_sockfd, in.c_str(), in.size(), 0);
        }
    private:
        int _sockfd;            // 重点是管理socket文件     
    };

}