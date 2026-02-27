#pragma once

#include <iostream>
#include <memory>

#include "Socket.hpp"
#include "Log.hpp"

using namespace SocketModule;
using namespace LogModule;

class Listener{
public:
    Listener(int port)
        :_listensock(std::make_unique<TcpSocket>())
        ,_port(port)
    {
        _listensock->BuildTcpSocketMethod(port);
    }

    void Accepter(){
        LOG(LogLevel::DEBUG) << "hhhhhh accepter";
    }

    int Sockfd() { return _listensock->Fd(); }

    ~Listener() { _listensock->Close(); }
private:
    std::unique_ptr<Socket> _listensock;    // 监听套接字
    int _port;  // 监听端口
};