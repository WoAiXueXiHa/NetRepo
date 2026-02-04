




















































































// #pragma once
// #include <istream>
// #include <string>
// #include <cerrno>
// #include <cstring>
// #include <functional>
// #include <pthread.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include "InetAddr.hpp"
// #include "Common.hpp"
// #include "Log.hpp"

// #define BACKLOG 8
// using namespace LogModule;

// class TcpServer{
//     // ==================================================
//     // 多线程版本添加内容
//     // 内部类：给线程传参
//     struct ThreadData{  
//         int _sockfd;            
//         TcpServer* self;         

//         ThreadData(TcpServer* t, int fd)
//             :self(t)
//             ,_sockfd(fd)
//             {}
//     };
//     // ==================================================
// public:
//     // -----------------------------------------------------
//     // 服务器心脏
//     // socket->bind->listen->accept->recv/send
//     // -----------------------------------------------------
//     TcpServer(int port = gport, int listenfd = gfd)
//         :_port(port)
//         ,_listensockfd(listenfd)
//         ,_isrunning(false)
//         {}
    
//     void InitServer(){
//             // 1. Socket 创建套接字
//             // AF_INET:IPv4
//             // SOCK_STREAM:TCP协议
//             // 返回值：_listensockfd迎宾员，唯一入口
//             _listensockfd = ::socket(AF_INET, SOCK_STREAM, 0);
//             if(_listensockfd < 0){
//                 LOG(FATAL) << "Socket create err! " << strerror(errno);
//                 exit(SOCKET_ERR);
//             }
//             LOG(INFO) << "Socket create success! fd: " << _listensockfd;
            
//             // 1.5 端口复用
//             // 允许服务器重启之后使用之前的端口，不用等TIME_WAIT结束
//             int opt = 1;
//             setsockopt(_listensockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        
//             // 2. Bind 挂招牌
//             // 生成绑定所需要的结构体
//             InetAddr local(_port);
            
//             int n = ::bind(_listensockfd, local.NetAddr(), local.NetAddrLen());
//             if(n < 0){
//                 LOG(FATAL) << "Bind create err! " << strerror(errno);
//                 exit(BIND_ERR);
//             }
//             LOG(INFO) << "Bind success! Port: " << _port;

//             // 3. Listen 拉客
//             // TCP是面向连接的，要求随时随地等待连接
//             // 门口排队超过8人，劝退后面来的
//             n = ::listen(_listensockfd, BACKLOG);
//             if(n < 0){
//                 LOG(FATAL) << "Listen err! " << strerror(errno);
//                 exit(LISTEN_ERR);
//             }
//             LOG(INFO) << "Listen success! Waiting for connections...";
//     }
    
//     void Service(int sockfd){
//         char buffer[1024];
//         while(1){
//             // 1> 读数据
//             // 和read类似
//             ssize_t n = ::recv(sockfd, buffer, sizeof(buffer) - 1, 0);
//             if(n > 0){  // 读到数据，处理字符串
//                 buffer[n] = '\0';
//                 LOG(INFO) << "Client[" << sockfd << "] says: " << buffer;
//                 // 2> 处理数据，简单回显
//                 std::string response = "Server Echo: " + std::string(buffer);
//                 // 3> 发数据
//                 ::send(sockfd, response.c_str(), response.size(), 0);
//             } else if(0 == n) {     // 读到文件结尾，对方下线了
//                 LOG(INFO) << "Client[" << sockfd << "] disconnected.";
//                 break;
//             } else{     // 出错了
//                 LOG(WARNING) << "recv err!";
//                 break;
//             }
//         }

//         // 6. 结束服务
//         // 文件描述符是有用的有限的资源
//         // 不关闭可能导致错误使用或fd泄漏！！！
//         ::close(sockfd);
//         LOG(INFO) << "Connection closed. fd: " << sockfd;
//     }
//     // ==================================================
//     // 多线程版本添加内容
//     static void* ThreadEntry(void* args){
//         // 1. 线程分离
//         // 临时工干完活自己走人，不需要回收
//         pthread_detach(pthread_self());
//         // 2. 拆包
//         ThreadData* td = static_cast<ThreadData*>(args);
//         // 3. 干活
//         td->self->Service(td->_sockfd);
//         // 4. 清理堆内存
//         delete td;

//         return nullptr;
//     }
//     // ==================================================

//     void Start(){
//         _isrunning = true;
//         while(_isrunning){
//             // 4. Accept 服务一个客人
//             // 这是一个阻塞函数，如果没有人来，程序阻塞等待
//             // 一旦有人来，内核会从队列里取出一个来，新建一个socket返回给你
//             struct sockaddr_in peer;
//             socklen_t peerlen = sizeof(peer);
           
//             // 注意：这里返回的是服务员！客人已经进店！
//             // 以后和客户沟通，全靠这个new_sockfd！
//             int new_sockfd = ::accept(_listensockfd, Conv(&peer), &peerlen);
//             if(new_sockfd < 0){
//                 LOG(WARNING) << "Accept err, continue...";
//                 // 注意，这里一个客人服务失败，无所谓，直接下一个客人就好！
//                 continue;
//             }
//             InetAddr clientAddr(peer);
//             LOG(INFO) << "Accept success! Client: " << clientAddr.AddrIp()
//                         << ", new sockfd: " << new_sockfd;
        
//             // 5. 提供服务
//             // // ==========================================================
//             // // Version 1: 多进程版核心逻辑 (Fork)
//             // pid_t id = fork();
//             // if(0 == id){
//             //     // 子进程
//             //     LOG(INFO) << "Child process working, pid: " << getpid();
//             //     // 子进程不需要监听，但是基础了父进程的_listensockfd
//             //     ::close(_listensockfd);
//             //     if(fork() > 0) exit(0); //子进程退出
//             //     // 孙子进程 -> 孤儿进程 -> 1
//             //     Service(new_sockfd);
//             //     exit(0);
//             // } else if(id > 0){
//             //     // 父进程
//             //     // 父进程只需要拉客
//             //     ::close(new_sockfd);
//             //     // 不会阻塞
//             //     int id = ::waitpid(id, nullptr, 0);
//             //     if(id < 0)
//             //     {
//             //         LOG(WARNING) << "Waitpid err！";
//             //     }
//             // } else {
//             //     LOG(WARNING) << "Fork err!";
//             //     ::close(new_sockfd);
//             // }
//             // // ==========================================================

            
//             // ----------------------------------------------------------
//             // 单进程阻塞版本
//             // Service(new_sockfd);
//             // ----------------------------------------------------------

//             // ==========================================================
//             // Version 2: 多线程版 (pthread_create)
//             // 必须是static 没有this指针
//             // 1. 打包参数
//             // 为什么不能用局部变量？
//             // 主线程循环极快，进入下一次循环时，局部变量就被销毁或覆盖了！
//             ThreadData* data = new ThreadData(this, new_sockfd);
//             // 2. 创建线程
//             pthread_t tid;
//             int n = pthread_create(&tid, nullptr, ThreadEntry, data);
//             if(0 != n){
//                 LOG(WARNING) << "Create thread err!";
//                 ::close(new_sockfd);
//                 delete data;
//             }
//             // ==========================================================
//         }
//     }

//     ~TcpServer(){
//         if(_listensockfd >= 0)
//             ::close(_listensockfd);
//     }
// private:
//     int _listensockfd;      // 迎宾员
//     int _port;
//     bool _isrunning;
// };