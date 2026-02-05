#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include "Common.hpp"

// [本地 Host]           [转换函数]            [网络/Socket Network]
// string "1.2.3.4"  --> inet_pton/inet_addr --> sin_addr (大端整数)
// uint16_t 8080     --> htons               --> sin_port (大端端口)
class InetAddr{
private:
    // 网络字节序转主机字节序 ntohs 
    void PortNet2Host() { _port = ::ntohs(_net_addr.sin_port); }
    // 网络IP转点分十进制字符串
    void IpNet2Host(){
        char ipbuffer[64];
        const char* ip = ::inet_ntop(AF_INET, &_net_addr.sin_addr,
                                     ipbuffer, sizeof(ipbuffer));
        _ip = ipbuffer;   
    }
public:
    InetAddr() {}
    ~InetAddr() {}

    // 场景1：接收到网络中的一个连接，用对方的sockaddr_in初始化这个类
    InetAddr(const struct sockaddr_in& addr)
        :_net_addr(addr)
        {
            PortNet2Host();
            IpNet2Host();
        }
    
    // 场景2：监听本地端口，传输到网络
    InetAddr(uint16_t port)
        :_port(port)
        ,_ip("")
        {
            _net_addr.sin_family = AF_INET;
            _net_addr.sin_port = ::htons(_port);
            _net_addr.sin_addr.s_addr = INADDR_ANY;
        }
    
    // 场景3：客户端连接指定的IP
    InetAddr(std::string ip = gip, uint16_t port = gport)
        :_ip(ip)
        ,_port(port)
        {
            memset(&_net_addr, 0, sizeof(_net_addr));
            _net_addr.sin_family = AF_INET;
            _net_addr.sin_port = ::htons(_port);
            int n = ::inet_pton(AF_INET, _ip.c_str(), &_net_addr.sin_addr);
            if(n < 0){
                std::cerr << "Invalid IP Address: " << _ip << std::endl;
            }
        }
    
    bool operator==(const InetAddr& addr){
        return _ip == addr._ip && _port == addr._port;
    }
    // 为了C的原生接口要求->bind,accept,connect
    struct sockaddr* NetAddr() { return Conv(&_net_addr); }
    socklen_t NetAddrLen() { return sizeof(_net_addr); }

    std::string Ip() const { return _ip; }
    uint16_t Port() const { return _port; }
    std::string PrintAddrIp() const { return Ip() + ":" + std::to_string(Port()); }
private:
    std::string _ip;          
    uint16_t _port;
    struct sockaddr_in _net_addr;
};