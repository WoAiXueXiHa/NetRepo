#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Log.hpp"
#include "Common.hpp"

using namespace LogModule;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8082

int main(int argc, char* argv[]){
    std::string ip = SERVER_IP;
    int port= SERVER_PORT;

    if(3 == argc){
        ip = argv[1];
        port = std::stoi(argv[2]);
    }

    // 客户端创建套接字
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        LOG(FATAL) << "Create socket err!";
        exit(SOCKET_ERR);
    }

    // 填写服务器的地址信息
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 发起连接
    // 客户端需要bind吗？
    // 不需要显式bind，客户端发起连接的适合，OS回随机分配一个端口给客户端
    int n = ::connect(sockfd, Conv(&server_addr), sizeof(server_addr));
    if(n < 0){
        LOG(FATAL) << "Connect err! Is the server running?";
        exit(CONNECT_ERR);
    }

    // 业务循环，聊天
    while(1){
        std::cout << "Please Enter# ";
        std::string msg;
        std::getline(std::cin, msg);

        if(msg == "quit") break;
        
        // 1> 写数据
        ssize_t n = ::write(sockfd, msg.c_str(), msg.size());
        if(n > 0){
            // 2> 读回显
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));

            // 阻塞等待服务器回应
            ssize_t m = ::read(sockfd, buffer, sizeof(buffer) - 1);
            if(m > 0){
                buffer[m] = '\0';
                LOG(INFO) << "Server Echo: " << buffer;
            }
            else if(0 == m){
                LOG(INFO) << "Server closed connection.";
                break;
            } else {
                LOG(WARNING) << "Client read err!";
                break;
            }
        } else {
            LOG(WARNING) << "Client write err!";
            break;
        }
    }
    // 关闭连接
    ::close(sockfd);
    return 0;
}