#pragma once
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONV(v) (struct sockaddr*)(v)
class InetAddr{
private:
    // 网络字节序端口->主机字节序
    void PortNet2Host() { _port = ::ntohs(_net_addr.sin_port); }
    // 网络字节序IP->字符串IP
    void IpNet2Host(){
        char ipbuffer[64];
        ::inet_ntop(AF_INET, &_net_addr.sin_addr, ipbuffer, sizeof(ipbuffer));
        _ip = ipbuffer;
    }
public:
    InetAddr() {}
    // 从收到的sockaddr_in 构建对象，用于recvfrom收到对方信息时
    InetAddr(const struct sockaddr_in& addr) 
        :_net_addr(addr)
        {
            PortNet2Host();
            IpNet2Host();
        }
    // 用于服务器bind本地端口时
    InetAddr(uint16_t port) 
        :_port(port)
        ,_ip("")
        {
            _net_addr.sin_family = AF_INET;
            _net_addr.sin_port = htons(_port);
            _net_addr.sin_addr.s_addr = INADDR_ANY;     // 监听本机所有IP
        }
    
    //  判断用户是不是已经在列表里了
    bool operator==(const InetAddr& addr){
        return _ip == addr._ip && _port == addr._port;
    }

    struct sockaddr* NetAddr() { return CONV(&_net_addr); }
    socklen_t NetAddrLen() { return sizeof(_net_addr); }
    std::string Addr() { return _ip + ":" + std::to_string(_port); }

private:
    struct sockaddr_in _net_addr;
    std::string _ip;
    uint16_t _port;
};