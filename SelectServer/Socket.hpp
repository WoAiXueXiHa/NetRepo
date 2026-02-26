#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"

namespace SocketModule{
    using namespace LogModule;
    class Socket{
    public:
        virtual ~Socket() = default;
        virtual void SocketOrDie() = 0;
        virtual void SetSocketOpt() = 0;
        virtual bool BindOrDie(int port) = 0;
        virtual bool ListenOrDie() = 0;
        virtual int Accepter(InetAddr *client) = 0;
        virtual void Close() = 0;
        virtual int Recv(std::string *out) = 0;
        virtual int Send(const std::string &in) = 0;
        virtual int Fd() = 0;

        // 提供一个创建TCP套接字的固定套路
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
        TcpSocket() : _sockfd(gdefaultsockfd) {}
        // 带着以及accept好的fd来构造->用于生成客户端通信的套接字
        TcpSocket(int sockfd) :_sockfd(sockfd) {}
        virtual ~TcpSocket() {}

        // 1. 创建套接字
        virtual void SocketOrDie() override{
            _sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
            if(_sockfd < 0){
                LOG(LogLevel::ERROR) << "socket err";
                exit(SOCKET_ERR);
            }
            LOG(LogLevel::DEBUG) << "socket create success: " << _sockfd;
        }

        // 2. 套接字选项设置
        virtual void SetSocketOpt() override{
            // 保证服务器异常断开之后可以立即重启
            int opt = 1;
            int n = ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            (void)n;
        }

        // 3. 绑定端口
        virtual bool BindOrDie(int port) override{
            if(_sockfd == gdefaultsockfd) return false;

            // 只需要传port，底层的结构体以及被InetAddr搞定了
            InetAddr addr(port);
            int n = ::bind(_sockfd, addr.NetAddr(), addr.NetAddrLen());
            if(n < 0){
                LOG(LogLevel::ERROR) << "bind error";
                exit(SOCKET_ERR);
            }
            LOG(LogLevel::DEBUG) << "bind create success: " << _sockfd;
            return true;
        }

        // 4. 开始监听
        virtual bool ListenOrDie() override{
            if(_sockfd == gdefaultsockfd) return false;
            int n = ::listen(_sockfd, gbacklog);
            if(n < 0){
                LOG(LogLevel::ERROR) << "listen error";
                exit(LISTEN_ERR);
            }
            LOG(LogLevel::DEBUG) << "listen create success: " << _sockfd;
            return true;
        }

        // 5. 获取新连接
        // 参数传入一个空的InetAddr，函数内部把客户端信息装进去
        int Accepter(InetAddr *client) override{
            struct sockaddr_in peer;    // OS原生结构体
            socklen_t len = sizeof(peer);

            // 阻塞式从内核全连接队列中拿出一个连接
            int newsockfd = ::accept(_sockfd, CONV(&peer), &len);
            if(newsockfd < 0){
                LOG(LogLevel::WARNING) << "accept error";
                return -1;
            }
            // 利用SetAddr方法，把原生结构体转换为InetAddr对象并带出
            client->SetAddr(peer, len);
            return newsockfd;
        }

        virtual void Close() override{
            if(_sockfd == gdefaultsockfd) return;
            ::close(_sockfd);
        }

        // 6. 网络IO读写
        virtual int Recv(std::string *out) override{
            char buffer[1024*8];
            // 默认阻塞读取
            ssize_t size = ::recv(_sockfd, buffer, sizeof(buffer) - 1, 0);
            if(size > 0){
                buffer[size] = '\0';
                *out = buffer;  // 赋值给C++string传出去
            }
            return size;
        }

        virtual int Send(const std::string &in) override{
            ssize_t size = ::send(_sockfd, in.c_str(), in.size(), 0);
            return size;
        }

        virtual int Fd() override { return _sockfd; }
    private:
        int _sockfd;
    };
}