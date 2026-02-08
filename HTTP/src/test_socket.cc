#include "Socket.hpp"
#include "InetAddr.hpp"
#include <iostream>
#include <unistd.h>

using namespace SocketModule;

int main() {
    std::cout << "--- TEST START ---" << std::endl;
    
    // 1. 创建对象 (多态)
    std::shared_ptr<Socket> sock = std::make_shared<TcpSocket>();
    
    // 2. 启动流程 (8080)
    sock->BuildTcpSocketMethod(8080);
    
    std::cout << "Server listening on 8080..." << std::endl;

    // 3. 尝试接受一个连接 (只接一个就退出，用于测试)
    InetAddr client_info;
    auto client = sock->Accepter(&client_info);
    
    if (client) {
        std::cout << "[SUCCESS] Connected: " << client_info.Addr() << std::endl;
        std::string hello = "Hello from your code!\n";
        client->Send(hello);
    }

    return 0;
}