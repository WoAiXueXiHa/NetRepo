#include <iostream>
#include <string>
#include "Log.hpp"
#include "Listener.hpp"
#include "Connect.hpp"
#include "EpollServer.hpp"

using namespace LogModule;

int main(int argc, char *argv[]){
    if(2 != argc){
        std::cout << "Usage: " << argv[0] << " port\n";
        return 1;
    }

    ENABLE_CONSOLE_LOG();
    uint16_t local_port = std::stoi(argv[1]);

    Listener listen(local_port);    // 完成工作的具体模块，也有一个listensockfd
    // 要把listensockf封装成一个Connecition，这个连接托管给EpollServer
    auto conn = std::make_shared<Connection>(listen.Sockfd());
    conn->InitCB([&listen](){
        listen.Accepter();
        }, nullptr, nullptr
    );
    conn->SetEvents(EPOLLIN | EPOLLET);

    EpollServer epoll_svr;
    epoll_svr.Init();
    epoll_svr.InsertConnetion(conn);
    epoll_svr.Loop();

    return 0;
}