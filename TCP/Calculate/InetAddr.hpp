#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <cstring>

class InetAddr{
private:
    // 字节序转换：网络转主机
    // 网络传过来大端序，主机不管是小段还是大端，做个转换最保险
    // ntohs
    void PortNet2Host() { _port = ::ntohs(_net_addr.sin_port); }

    // IP格式转换 二进制->字符串
    // 网络传过来32位整数，要转成人看得懂的点分十进制字符串
    // inet_ntop
    void IpNet2Host(){
        char ipbuffer[64];
        ::inet_ntop(AF_INET, &_net_addr.sin_addr, ipbuffer, sizeof(ipbuffer));
        _ip = ipbuffer;
    }
public:
    // 场景1：作为接收方，accept时使用
    // OS accept 了一个连接，扔给你一块生肉sockaddr_in
    // 现在需要把生肉塞进InetAddr，翻译成人能懂的IP和Port
    InetAddr(const struct sockaddr_in& addr)
        :_net_addr(addr)
        {
            PortNet2Host();
            IpNet2Host();
        }
    
    // 场景2：作为发送方/监听方 bind/connect时用
    // 把人看得懂的IP和Port 填进 struct sockaddr_in
    InetAddr(std::string ip, uint16_t port)
        :_ip(ip)
        ,_port(port)
        {
            // 这一步很重要，防止脏数据
            memset(&_net_addr, 0, sizeof(_net_addr));
            _net_addr.sin_family = AF_INET;
            _net_addr.sin_port = ::htons(_port);
            ::inet_pton(AF_INET, _ip.c_str(), &_net_addr.sin_addr);
        }

    // 场景3：监听本地窗口,传输到网络
    InetAddr(uint16_t port)
        :_port(port)
        ,_ip("")
        {
            _net_addr.sin_family = AF_INET;
            _net_addr.sin_port = ::htons(_port);
            _net_addr.sin_addr.s_addr = INADDR_ANY;
        }

    // 为了配合C接口bind/accept
    // 需要 struct sockaddr* 原生指针
    struct sockaddr* NetAddr() { return (struct sockaddr*)&_net_addr; }
    socklen_t NetAddrLen() { return sizeof(_net_addr); }

    std::string Ip() const { return _ip; }
    uint16_t Port() const { return _port; }
private:
    std::string _ip;
    uint16_t _port;
    struct sockaddr_in _net_addr;       // 系统原生的结构体
};