#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "Protocol.hpp"

using namespace ProtocolModule;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
        return 1;
    }

    // 连接服务器 
    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    ::inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 3;
    }
    std::cout << "Connect Success!" << std::endl;

    // 重点1：客户端也要有蓄水池！
    std::string in_buffer; 

    while (true) {
        Request req;
        std::cout << "Please Enter (x op y): ";
        std::cin >> req._x >> req._op >> req._y;
        if (!std::cin.good()) break;

        // 发送流程：序列化 -> 封包 -> 发送
        std::string json_str;
        req.Serialize(&json_str);     
        std::string packet = Encode(json_str); 
        ::write(sockfd, packet.c_str(), packet.size());

        // 重点2：接收响应不能只 read 一次！
        // 必须循环读，直到 Decode 出一个完整包为止
        char buffer[1024];
        while (true) {
            ssize_t n = ::read(sockfd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = 0;
                in_buffer += buffer; // 拼接

                std::string payload = Decode(in_buffer); // 尝试切割
                if (!payload.empty()) {
                    // 拿到完整包了，反序列化并打印
                    Response resp;
                    resp.Deserialize(payload);
                    std::cout << "Result: " << resp._result << " [Code: " << resp._code << "]" << std::endl;
                    
                    // 拿到结果后，跳出内层循环，准备下一次用户输入
                    break; 
                }
            } else {
                goto END;
            }
        }
    }

END:
    ::close(sockfd);
    return 0;
}