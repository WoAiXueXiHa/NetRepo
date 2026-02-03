#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Common.hpp"
#include "Log.hpp"
using namespace LogModule;

void Usage(std::string proc){
    std::cout << "Uasge: " << proc << "server_ip server_port" << std::endl; 
}

int main(int argc, char* argv[]){
    if(3 != argc){
        Usage(argv[0]);
        LOG(ERROR) << "Usage error" << argv[0];
        return USAGE_ERR;
    }

    // 1. 获取服务器的IP和端口
    std::string server_ip = argv[1];
    uint16_t server_port = std::stoi(argv[2]);

    // 2. 创建套接字
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0){
        LOG(ERROR) << "Create socket error: " << strerror(errno);
        exit(SOCKET_ERR);
    }

    // clinet 要不要 bind？
    // client需要端口号才能通信，所以一定要bind
    // 但是！我们不需要显式调用bind
    // why？
    // 如果强制绑定8080端口，那么电脑上就只能开一个客户端，开第二个就报错端口占用
    // 客户端不需要固定的端口，只要能把消息发出去就好
    // 什么时候bind？
    // 在第一次调用 sendto 时，OS会自动给当前 socket 分配一个
    // 随机的、未被占用的端口号
    
    // 3. 填充要发给服务器的信息
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(server_ip.c_str());

    while(1){
        std::string line;
        std::cout << "Please Enter# ";
        std::getline(std::cin, line);

        if(!std::cin.good()) break;

        // 4. 发消息
        // 这里不需要bind，OS会自动分配端口
        int n = ::sendto(sock, line.c_str(), line.size(), 0, CONV(&server), sizeof(server));
        if(n > 0){
            // 5. 接收回显
            // 虽然我们知道时服务器发来的消息，但是recvfrom接口要求必须填这两个参数
            // 可以定义一个temp结构体占位
            struct sockaddr_in temp;
            socklen_t len = sizeof(temp);
            char buffer[1024];
            ssize_t s = ::recvfrom(sock, buffer, sizeof(buffer) - 1, 0, CONV(&temp), &len);
            if(s > 0){
                buffer[s] = '\0';
                LOG(INFO) << "Server echo# " << buffer;
            }
        }
    }

    ::close(sock);
    return 0;
}
