#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1" // 本地回环测试

int main() {
    // 1. 创建套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }

    // 2. 填充目标（服务器）地址信息
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    // 将字符串IP转换为网络字节序
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP); 

    // 3. 直接发送 (Sendto)
    const char *hello = "Hello from Client";
    // 注意：UDP是无连接的，必须在发的时候指定发给谁 (&servaddr)
    sendto(sockfd, (const char *)hello, strlen(hello), 
           0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    std::cout << "Message sent." << std::endl;

    // 4. 接收回复 (Recvfrom)
    char buffer[1024];
    socklen_t len = sizeof(servaddr);
    
    // 这里也可以传 NULL，如果你不关心是谁回你的（但在UDP中最好校验源地址）
    int n = recvfrom(sockfd, (char *)buffer, 1024, 
                     0, (struct sockaddr *)&servaddr, &len);
    
    buffer[n] = '\0';
    std::cout << "Server : " << buffer << std::endl;

    close(sockfd);
    return 0;
}