#pragma once
#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
// ------------------ 网络四件套头文件 -----------------
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Log.hpp"
#include "NoCopy.hpp"
#include "Common.hpp"
#include "InetAddr.hpp"
using namespace LogModule;

// 默认配置
const static uint16_t defaultport = 8080;
const static int defaultfd = -1;
const static int defaultsize = 1024;

class UdpServer : public NoCopy{
public:
    UdpServer(uint16_t port = defaultport, int sockfd = defaultfd)
        :_port(port)
        ,_sockfd(sockfd)
        {}
    ~UdpServer(){
        if(_sockfd >= 0)
            ::close(_sockfd);
    }
    
    // 初始化：1.找系统分配一个网卡接口（socket） 2. 占个坑位等待绑定
    void Init(){
        // 1. 创建套接字
        // AF_INET: IPv4协议族
        // SOCK_DGRAM: UDP协议，不可靠但快
        // 0: 默认协议
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(_sockfd < 0){
            LOG(FATAL) << "Socket create error: " << strerror(errno);
            exit(SOCKET_ERR);
        }
        // 创建成功
        LOG(INFO) << "Socket create success, sockfd: " << _sockfd;

        // 2. 填充bind需要的结构体信息
        struct sockaddr_in local; 
        // 填充之前先清0，sin_zero字段可能有垃圾值
        bzero(&local, sizeof(local));
        local.sin_family = AF_INET;     // 和创建socket时一样
        // 端口号必须转成网络字节序htons
        // h(host) to n(network) s(short/16bits)
        local.sin_port = htons(_port);
        // 这台机器上所有网卡收到的、发到808端口的数据，我都要
        // 这里需要注意sin_addr是个结构体，结构体里只有一个对象s_addr
        // 结构体只能整体初始化，不能整体赋值，只能指定对象逐一赋值
        local.sin_addr.s_addr = INADDR_ANY;

        // 3. 绑定
        // bind的接口时通用的 第二个参数需要struct sockaddr* 类型
        int n = ::bind(_sockfd, CONV(&local), sizeof(local));
        if(0 != n){
            LOG(FATAL) << "Bind error: " << strerror(errno);
        }
        LOG(INFO) << "UdpServer init success, port: " << _port;
    }

    // 启动服务：死循环收发数据
    void Start(){
        char buffer[defaultsize];
        while(1){
            // peer存：是谁发给我的？对端IP和port
            struct sockaddr_in peer;
            // len必须初始化，len是一个输出型参数
            socklen_t len = sizeof(peer);

            // 4. 接收数据 recvfrom
            // 这是一个阻塞函数，每人发消息，程序就阻塞等待，不占用CPU
            ssize_t n = ::recvfrom(_sockfd, buffer, sizeof(buffer) - 1, 0, CONV(&peer), &len);
            if(n > 0){
                // 5. 处理数据
                // 手动添加结束符
                buffer[n] = '\0';

                // 我需要知道是谁发给我的消息
                // inet_ntoa:把数字IP转成"192.188.1.1"这种形式
                // ntohs:把网路端口转成主机端口
                // std::string client_ip = inet_ntoa(peer.sin_addr);
                // uint16_t client_port = ntohs(peer.sin_port);
                InetAddr addr(peer);

                // LOG(INFO) << "Get message from to [" << client_ip << ":" << client_port << "]# " << buffer;
                LOG(INFO) << "Get messages from to [" << addr.PrintDebug() << "]#" << buffer;
                // 6. 发回消息
                sendto(_sockfd, buffer, strlen(buffer), 0, CONV(&peer), len);
                LOG(INFO) << "Echo back to [" << addr.PrintDebug() << "]";
            }else{
                LOG(WARNING) << "Recvfrom warning: " << strerror(errno);
            }
        }
    }
private:
    uint16_t _port;
    // 服务器不需要IP地址
    int _sockfd;
};