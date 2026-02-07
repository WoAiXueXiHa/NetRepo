#pragma once
#include <iostream>
// 网络四件套头文件
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Common.hpp"

class InetAddr{
private:
    // 网络字节序转主机字节序
    void PortNet2Host(){ _port = ::ntohs(_net_addr.sin_port); }
    // 网络IP转点分十进制字符串
    void IpNet2Host(){
        char ipbuffer[64];
        const char *ip = ::inet_ntop(AF_INET, &_net_addr.sin_addr, ipbuffer, sizeof(ipbuffer));
        _ip = ipbuffer;
    }
public:
    InetAddr(){}
    ~InetAddr(){}
    
    // 场景一：接收到一个连接，用对方的sockaddr_in初始化这个类
    InetAddr(const struct sockaddr_in& addr)
    	:_net_addr(addr)
    {
        PortNet2Host();
        IpNet2Host();
    }
    
    // 场景二：监听本地端口
    InetAddr(uint16_t port) 
        : _port(port)
        ,_ip("")
      	{
        	_net_addr.sin_family = AF_INET;
            _net_addr.sin_port = htons(_port);	// 主机转网络序
            _net_addr.sin_addr.s_addr = INADDR_ANY;
    	}
    
    bool operator==(const InetAddr& addr){
        return _ip == addr._ip && _port == addr._port;
    }
   
    // #define Conv(v) (struct sockaddr*)(v)
    struct sockaddr* NetAddr() { return Conv(&_net_addr); }
    socklen_t NetAddrLen() { return sizeof(_net_addr); }
    std::string Ip() { return _ip; }
    uint16_t Port() { return _port; }
    std::string AddrIp() { return Ip() + ":" + std::to_string(Port()); }
private:
    uint16_t _port;						// 主机端口号
    std::string _ip;					// 用户可读IP地址
    struct sockaddr_in _net_addr;		// 系统底层地址结构体
};