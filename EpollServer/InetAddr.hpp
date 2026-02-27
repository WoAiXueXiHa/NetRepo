#pragma once

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Common.hpp"


class InetAddr{
private:
    // 内部私有方法：网络大端字节序（网络看）转回主机字节序（用户看）
    void PortNet2Host(){ _port = ::ntohs(_net_addr.sin_port); }
    // 内部私有方法：32位网络IP转成点分十进制字符串
    void IpNet2Host(){
        char ipbuffer[64];
        const char *ip = ::inet_ntop(AF_INET, &_net_addr.sin_addr, ipbuffer, sizeof(ipbuffer));
        (void)ip;
        _ip = ipbuffer;
    }

public:
    InetAddr(){}

    // 场景1：服务器accept到了一个新客户端
    // OS甩给我一个sockaddr_in,我丢给构造函数解析出IP和端口号
    InetAddr(const struct sockaddr_in &addr)
        :_net_addr(addr)
    {
        PortNet2Host();
        IpNet2Host();
    }

    // 场景2：服务器初始化，准备bind
    // 我只用告诉服务器，“我要绑定哪个端口”，它内部帮我把sockaddr_in他Inman，并转成网络字节序
    InetAddr(uint16_t port)
        :_port(port)
        ,_ip("")
    {
        _net_addr.sin_family = AF_INET;
        _net_addr.sin_port = ::htons(_port);
        _net_addr.sin_addr.s_addr = INADDR_ANY; // 绑定本机所有网卡IP
    }

    // 判断相等，方便后续维护连接池，判断是否为用一个客户端
    bool operator==(const InetAddr &addr){
        return _ip == addr._ip && _port == addr._port;
    }

    // 暴露底层API使用接口：将内部的sockaddr_in取出，强转为sockaddr*
    struct sockaddr *NetAddr() { return CONV(&_net_addr); }
    socklen_t NetAddrLen() { return sizeof(_net_addr); }

    // 暴露给上层业务使用的接口：直接获取IP和端口
    std::string Ip() { return _ip; }
    uint16_t Port() { return _port; }

    // 拼接点分十进制，降低打印日志成本
    std::string Addr(){
        return Ip() + ":" + std::to_string(Port());
    }

    // 复用已存在的InetAddr对象，更新他的内部数据
    void SetAddr(const sockaddr_in &client, socklen_t &len){
        _net_addr = client;
        (void)len;
        IpNet2Host();
        PortNet2Host();
    }

    ~InetAddr(){}
private:
    struct sockaddr_in _net_addr;   // 系统视角的底层基石
    std::string _ip;
    uint16_t _port;
};