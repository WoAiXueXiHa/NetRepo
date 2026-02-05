#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "InetAddr.hpp"
#include "Log.hpp"
using namespace LogModule;

// ./tcp_client 127.0.0.1 8082
int main(int argc, char* argv[]){
    if(3 != argc){
        std::cerr << "Usage: " << argv[0] << "<server_ip> <server_port>" << std::endl;
        return 10;
    }

    std::string server_ip = argv[1];
    uint16_t server_port = std::stoi(argv[2]);

    // 1. 创建socket
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        LOG(FATAL) << "Socket create failed!";
        exit(SOCKET_ERR);
    }
    LOG(INFO) << "Client socket create success!";

    // 2. 构建服务器地址信息
    InetAddr server_addr(server_ip,server_port);

    // 3. 发起连接
    int n = ::connect(sockfd, server_addr.NetAddr(), server_addr.NetAddrLen());
    if(n < 0){
        LOG(FATAL) << "Connect failed!";
        ::close(sockfd);
        exit(CONNECT_ERR);
    }
    LOG(INFO) << "Connect success! Starting chatting...";

    // 4. 业务循环
    while(1){
        std::cout << "Please Enter# ";
        std::string msg;
        if(!std::getline(std::cin, msg)) break;
        if(msg == "quit") break;

        // 发给服务器
        ssize_t n = ::write(sockfd, msg.c_str(), msg.size());
        if(n < 0){
            std::cerr << "Write error" << std::endl;
            break;
        }

        // 接收服务器回显
        char buffer[1024];
        n = ::read(sockfd, buffer, sizeof(buffer) - 1);
        if(n > 0){
            buffer[n] = '\0';
            std::cout << "Server Echo: " << buffer << std::endl;
        } else if(0 == n){
            std::cout << "Server closed connection." << std::endl;
            break;
        }else{
            std::cerr << "Read error" << std::endl;
            break;
        }
    }

    // 5. 关闭资源
    ::close(sockfd);
    return 0;
}