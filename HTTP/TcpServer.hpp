#pragma once

#include <iostream>
#include <memory>
#include <functional>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Socket.hpp"

namespace TcpServerModule{
    using namespace SocketModule;

    // 定义业务处理函数
    using tcp_handler_t = std::function<void(SockPtr, InetAddr)>;

    class TcpServer{
    public:
        TcpServer(uint16_t port)
            :_port(port)
            ,_listensock(std::make_unique<TcpSocket>())
            ,_running(false)
            {}

        // 初始化服务器
        void InitServer(tcp_handler_t handler){
            _handler = handler;
            // 调用写好的模板方法
            _listensock->BuildTcpSocketRule(_port);
        }

        // 启动服务器
        void Loop(){
            // 僵尸进程处理
            // 子进程退出时，会向父进程发送SIGCHLD信号
            // 让内核回收子进程，父进程别写waitpid
            // signal(SIGCHLD, SIG_IGN);

            _running = true;
            std::cout << "TcpServer loop running on port " << _port << "..." << std::endl;

            while(_running){
                // 1. 获取新连接
                InetAddr client_info;
                SockPtr service_sock = _listensock->Accept(&client_info);
                if(service_sock == nullptr) continue;
                std::cout << "Get a new connection: " << client_info.Addr() << std::endl;

                // 2. 创建孙子进程实现并发
                pid_t id = fork();
                if(0 == id){
                     // ==== 儿子进程 ====
                     // 儿子不需要监听
                    _listensock->Close();

                    // 创建孙子进程并立刻自杀
                    if(fork() > 0) exit(0);

                    // ==== 孙子进程 ====
                    // 让孙子进程执行，现在孙子进程是孤儿进程了，1号进程回收

                    // 执行具体业务逻辑
                    if(_handler) { _handler(service_sock, client_info); }

                    // 业务执行完就退出
                    exit(0);
                } else if(id > 0) {
                    // 爷爷进程只负责拉客
                    // 这里的 service_sock 是智能指针
                    // 发生了引用计数拷贝：爷爷进程持有 1 份，孙子进程拷贝持有 1 份，Ref=2
                    // 爷爷进程如果不把手里这份释放（Close 或 智能指针析构），Ref 永远不会归零
                    // 导致 TCP 连接断不开（文件描述符泄漏）
                    // 爷爷必须等待儿子退出
                    // 儿子进去就fork一下就退出，这个waitpid几乎不耗时
                    waitpid(id, nullptr, 0);
                    // 注意：因为 service_sock 是局部变量，出了本次 while 循环作用域
                    // 它的智能指针会自动析构，引用计数 -1
                    // 所以对于 shared_ptr，这里其实不用显式写 close
                    service_sock->Close();
                } else {
                    std::cerr << "fork error" << std::endl;
                }
            }
        }

        ~TcpServer() {}
    private:
        uint _port;
        std::unique_ptr<Socket> _listensock;
        bool _running;
        tcp_handler_t _handler;     // 回调任务
    };
}