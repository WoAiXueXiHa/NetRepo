#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1" // 本地回环测试

int main() {
    // 1. 创建套接字 socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        std::cerr << "socket create failed: " << errno << std::endl;
        exit(-1);
    }
    std::cout << "socket create success! fd: " << sockfd << "\n";

    // 2. 填充目标服务器地址信息
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ::inet_addr(SERVER_IP);
    server_addr.sin_port = ::htons(PORT);

    // 3. 直接发送信息 sendto
    const char* hello = "Hello from client";
    ::sendto(sockfd, hello, strlen(hello),
             0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    std::cout << "Msg sent\n";

    // 4. 接收回复
    char buffer[1024];
    socklen_t len = sizeof(server_addr);
    int n = ::recvfrom(sockfd, buffer, 1024,
                       0, (struct sockaddr*)&server_addr, &len);
    if(n > 0) {
        buffer[n] = '\0';
        std::cout << "server: " << buffer << std::endl;
    } 
    
    ::close(sockfd);
    return 0;
}