#include "UdpServer.hpp"
#include <memory>

int main(int argc, char* argv[]){
    if(2 != argc){
        LOG(ERROR) << "Usage error";
        return USAGE_ERR;
    }

    ENABLE_CONSOLE_LOG();

    // 1. 拿到端口号
    uint16_t port = std::stoi(argv[1]);

    // 2. 创建服务器对象
    std::unique_ptr<UdpServer> svr(new UdpServer(port));

    // 3. 初始化
    svr->Init();

    // 4. 启动
    svr->Start();

    return 0;
}