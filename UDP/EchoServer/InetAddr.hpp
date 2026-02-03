#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class InetAddr{
public:
    InetAddr(struct sockaddr_in& addr)
        :_addr(addr)
        {
            // 网络序->主机序
            _port = ntohs(_addr.sin_port);
            // 注意：inet_ntoa返回的时静态内存地址，连续调用两次，第二次结果会覆盖第一次
            // 赋值给了std::string _ip 发生深拷贝
            _ip = inet_ntoa(_addr.sin_addr);
        }

    std::string Ip() { return _ip; }
    uint16_t Port() { return _port; }

    std::string PrintDebug(){
        std::string info = _ip;
        info += ":";
        info += std::to_string(_port);
        return info;
    }

    const struct sockaddr_in& GetAddr(){ return _addr; }
    ~InetAddr(){}
private:
    std::string _ip;
    uint16_t _port;
    struct sockaddr_in _addr;
};