#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(){
    // 1. 创建套接字 socket
    // AF_INET: IPv4
    // SOCK_DGRAM: UDP数据报类型
    // 0: 默认
    int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        std::cerr << "socket create err" << errno << std::endl;
        return -1;
    }

    // 2. 填充服务器地址结构体
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));   // 清零，防止垃圾数据
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   // 接收任意网卡的连接
    server_addr.sin_port = ::htons(PORT);   // 端口转网络字节序

    // 3. 绑定 bind
    // 必须绑定，否则客户端不知道发给谁
    if(::bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        std::cerr << "bind err" << errno << std::endl;
        return -1;
    }
    std::cout << "UDP Server is running on port " << PORT << "..." << std::endl;

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;         // 必须保护客户端的信息
    socklen_t len = sizeof(client_addr);    // 必须初始化为结构体的大小
    while(1){
        // 4. 接收数据 recvfrom
        // 最后两个参数是输出型参数，内核会把"谁发的"填进去
        int n = ::recvfrom(sockfd, (char*)buffer, BUFFER_SIZE,
                           0, (struct sockaddr*)&client_addr, &len);
        buffer[n] = '\0';
        std::cout << "Client : " << buffer << std::endl;

        // 5. 发送回复 sendto
        const char* hello = "hello from server";
        ::sendto(sockfd, (const char*)hello, strlen(hello),
                 0, (const struct sockaddr*)&client_addr, len);
    }
    close(sockfd);

    return 0;
}