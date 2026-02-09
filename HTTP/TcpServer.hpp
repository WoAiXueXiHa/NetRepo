#pragma once

#include <iostream>
#include <functional>
#include <memory>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "Socket.hpp"
#include "InetAddr.hpp"

namespace TcpServerModule{
    using namespace SocketModule;
    
    // 回调函数：业务处理函数
    using tcphandler_t = std::function<void(SockPtr, InetAddr)>;

    class TcpServer{
    public:
        TcpServer(uint16_t port)
            :_port(port)
            ,_is_running(false)
            // 先创建一个TcpSocket对象
            ,_listensock(std::make_shared<TcpSocket>())
            {}
        
            // 1. 初始化
            void InitServer(tcphandler_t handler){
                _handler = handler;
                // 复用Socket里的模板方法
                // 一步完成bind和listen，防止出错
                _listensock->BuildTcpSocketMethod(_port);
                LOG(INFO) << "TcpServer Init Success on port: " << _port << std::endl;
            }

            // 2. 业务循环
            void Loop(){
                _is_running = true;
                LOG(INFO) << "TcpServer Loop Starting..." << std::endl;

                while(_is_running){
                    InetAddr client_addr;
                    // 1. 获取连接，阻塞等待
                    SockPtr client_sock = _listensock->Accepter(&client_addr);

                    if(client_sock == nullptr) continue;    // 连接失败就下一个

                    LOG(INFO) << "Get a new link: " << client_addr.Addr() << std::endl;

                    // 2. 创建孤儿进程处理并发
                    pid_t id = fork();
                    if(0 == id){
                        // =================== 子进程 ===================
                        _listensock->Close();
                        if(fork() > 0) exit(0);
                        // =================== 孙子进程 ===================
                        if(_handler) _handler(client_sock, client_addr);
                        exit(0);
                    }
                    // =================== 爷爷进程 ===================
                    client_sock->Close();
                    waitpid(id, nullptr, 0);
                }
                _is_running = false;
            }

            ~TcpServer(){}
    private:
        uint16_t _port;
        bool _is_running;
        SockPtr _listensock;
        tcphandler_t _handler;
    };
}