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
        std::cerr << "socket create failed: " << errno << std::endl;
        exit(-1);
    }
    std::cout << "socket create success! fd: " << sockfd << "\n";
    // 2. 填充服务器地址结构体
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = ::htons(PORT);

    // 3. 绑定服务器 bind
    int n = ::bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(n < 0){
        std::cerr << "bind failed: " << errno << std::endl;
        exit(-1);
    }
    std::cout << "UDP server is running on port " << PORT << std::endl;

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    // 4. 接收数据 recvfrom
    // 最后两个参数是输出型参数，内核会把"谁发的"填进去
    while(1){
        int n = ::recvfrom(sockfd, buffer, BUFFER_SIZE,
                           0, (struct sockaddr*)&client_addr, &len);
        if(n > 0) {
            buffer[n] = '\0';
            std::cout << "Clinet : " << buffer << std::endl;
        }
        else if(0 == n) {
            std::cout << "Received empty packet\n";
        } else {
            std::cerr << "recv err\n";
            break;
        }
        // 5. 发送回复 sendto
        const char* hello = "Hello from server";
        ::sendto(sockfd, hello, strlen(hello),
                 0, (struct sockaddr*)&client_addr, len);
    }  

    ::close(sockfd);

    return 0;
}