#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Common.hpp"

class InetAddr{
private:
    void PortNet2Host() { _port = ::ntohs(_net_addr.sin_port); }
    void IpNet2Host(){
        char ipbuffer[64];
        ::inet_ntop(AF_INET, &_net_addr.sin_addr, ipbuffer, sizeof(ipbuffer));
        _ip = ipbuffer;
    }
public:
    InetAddr() {}
    // 给服务器监听，把本机数传输到网络，参数只要一个端口号
    InetAddr(uint16_t port) 
        : _port(port)
        ,_ip("")
        {
            memset(&_net_addr, 0, sizeof(_net_addr));
            _net_addr.sin_family = AF_INET;
            _net_addr.sin_port = htons(_port);
            _net_addr.sin_addr.s_addr = INADDR_ANY;
        } 
        
    // 给accept获取客户端信息用
    // 接收一个系统填充好的sockaddr_in，转成人能看懂的IP字符串
    InetAddr(const struct sockaddr_in& addr)
        :_net_addr(addr)
        {
            PortNet2Host();
            IpNet2Host();
        }
    
    // 获取底层结构体指针，bind()系统调用需要
    struct sockaddr* NetAddr() { return Conv(&_net_addr); }
    socklen_t NetAddrLen() { return sizeof(_net_addr); } 

    std::string Ip() const { return _ip; }
    uint16_t Port() const { return _port; }

    // 方便打印日志
    std::string Addr(){ return Ip() + ":" + std::to_string(Port()); }

    // accept后，把远端信息太难如这个对象
    void SetAddr(const struct sockaddr_in& client){
        _net_addr = client;
        IpNet2Host();
        PortNet2Host();
    }
private:
    std::string _ip;
    uint16_t _port;
    struct sockaddr_in _net_addr;
};