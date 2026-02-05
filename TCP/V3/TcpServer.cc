#include <iostream>
#include <memory>
#include "TcpServer.hpp"
#include "Log.hpp"


using namespace ThreadPoolModule;
using namespace LogModule;

void EchoService(int sockfd, InetAddr clinet_addr){
    LOG(INFO) << "Start processing client: " << clinet_addr.PrintAddrIp();
    char buffer[1024];
    while(1){
        // 1. 接收数据
        ssize_t n = ::read(sockfd, buffer, sizeof(buffer) - 1);
        if(n > 0){
            buffer[n] = '\0';
            std::cout << "Recv from [ " << clinet_addr.PrintAddrIp() << "]: " << buffer << std::endl;
            
            std::string response = "[Server Echo]: " + std::string(buffer);

            // 2. 发送数据
            ::write(sockfd, response.c_str(), response.size());
        } else if(0 == n) {
            LOG(INFO) << "Client " << clinet_addr.PrintAddrIp() << "disconnected";
            break;
        } else {
            LOG(ERROR) << "Read err, errno: " << errno;
            break;
        }
    }
    // 退出循环 lambda会close(sockfd)
}

int main(int argc, char* argv[]){
    if(2 != argc){
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 10;
    }

    // 1. 解析端口
    uint16_t port = std::stoi(argv[1]);
    // 2. 实例化服务器对象
    std::unique_ptr<TcpServer> server(new TcpServer(port));
    // 3. 初始化
    server->InitServer();
    // 4. 启动服务
    server->Loop(EchoService);
    
    return 0;
}