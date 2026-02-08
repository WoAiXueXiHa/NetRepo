#pragma once
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class InetAddr{
public:
    InetAddr() {}
    // 自己做服务器监听时使用
    InetAddr(uint16_t port)
        :_port(port)
        ,_ip("")    
        {
            _net_addr.sin_family = AF_INET;
            // 我们自己做服务器，要把端口发到网络中，等待连接
            _net_addr.sin_port = ::htons(_port);
            // IP接收本地所有网卡的连接
            _net_addr.sin_addr.s_addr = INADDR_ANY;

        }
    // accept收到客户端连接时使用
    // OS给了我们一个填充好的sockaddr_in，要翻译出IP和Port
    InetAddr(const struct sockaddr_in& addr)
        :_net_addr(addr)
        {
            _port = ::ntohs(addr.sin_port);
            IpNet2Host();
        }

    // 获取底层结构体指针和大小，给bind和accept使用
    struct sockaddr* NetAddr(){ return (struct sockaddr*)&_net_addr; }
    socklen_t NetAddrLen() { return sizeof(_net_addr); }
    // 方便打印日志
    std::string Addr() { return _ip + ":" + std::to_string(_port); }

    // accept之后用来填回数据
    // 拿到网络中系统填充好的struct sockaddr_in对象
    void SetAddr(const struct sockaddr_in& client, socklen_t& len){
        _net_addr = client;
        _port = ::ntohs(_net_addr.sin_port);
        IpNet2Host();
    }
private:
    // helpper
    void IpNet2Host(){
        char ipbuffer[64];
        const char* ip = inet_ntop(AF_INET, 
                                  &_net_addr.sin_addr, 
                                  ipbuffer,
                                  sizeof(ipbuffer));
        if(ip) _ip = ipbuffer;
    }

    uint16_t _port;
    std::string _ip;
    struct sockaddr_in _net_addr;
};