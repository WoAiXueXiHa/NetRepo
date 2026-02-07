#include "TcpServer.hpp"
#include <memory>

using namespace LogModule;

int main(){
    // 1. 实例化服务器对象
    std::unique_ptr<TcpServer> tsvr(new TcpServer(8082));
    // 2. 初始化服务器
    tsvr->InitServer();
    // 3. 启动服务
    tsvr->Start();

    return 0;
}