#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <sys/select.h>
#include "Log.hpp"
#include "Socket.hpp"
#include "Common.hpp"

using namespace SocketModule;
using namespace LogModule;

#define NUM sizeof(fd_set) * 8

class SelectServer{
public:
    SelectServer(int port)
        :_port(port)
        ,_listen_socket(std::make_unique<TcpSocket>())
        ,_is_running(false)
        {}

    void Init(){
        // 直接复用封装好的Socket
        _listen_socket->BuildTcpSocketMethod(_port);

        // 把辅助数组初始化为-1，代表没人用过
        for(int i = 0; i < NUM; i++) _fd_array[i] = gdefaultsockfd;

        // 服务器刚启动，只有监听套接字在工作
        // 必须把监听放到辅助数组的0号位置，它是服务器存活的基础
        _fd_array[0] = _listen_socket->Fd();
    }

    void Accepter(){
        InetAddr client;
        // 1. 获取新连接，此时accept一定不会阻塞！因为select已经确定有人来了！
        int newfd = _listen_socket->Accepter(&client);
        if(newfd < 0) return;

        std::cout << "获得了一个新的连接: " << newfd << " client info: " << client.Addr() << std::endl;

        // 2. 寻找辅助数组里的空位
        int pos = -1;
        for(int j = 0; j < NUM; j++){
            if(_fd_array[j] == gdefaultsockfd){
                pos = j;
                break;  // 找到第一个空位就停下
            }
        }

        // 3. 登记入册或拒绝服务
        if(pos == -1){
            // 辅助数组满了，服务器负载到极限
            LOG(LogLevel::ERROR) << "服务器满了...";
            close(newfd);
        } else {
            // 成功等级，在下一次Loop循环中，这个newfd会被加入到rfds里被内核监控
            _fd_array[pos] = newfd;
        }
    }

    void Recver(int who){
        char buffer[1024];
        // 此时recv绝对不会阻塞，因为是被Dispatcher确认过有数据的fd
        ssize_t n = ::recv(_fd_array[who], buffer, sizeof(buffer) - 1, 0);

        if( n > 0){
            // 正常读到了数据
            buffer[n] = '\0';
            std::cout << "client# " << buffer << std::endl;

            // 简单回显
            std::string msg = "echo# ";
            msg += buffer;
            // 这里注意：send真的不会阻塞吗，如果发送缓冲区满了怎么办？
            // 还可以优化，后续做
            ::send(_fd_array[who], msg.c_str(), msg.size(), 0);
        } else if(0 == n){
            // 客户端挥手退出
            LOG(LogLevel::DEBUG) << "客户端退出， sockfd: " << _fd_array[who];

            // 在OS层面释放套接字
            ::close(_fd_array[who]);

            // 在用户态将辅助数组除名
            _fd_array[who] = gdefaultsockfd;
        } else {
            // n < 0 可能是网络波动导致物理断开
            LOG(LogLevel::DEBUG) << "客户端读取出错， sockfd: " << _fd_array[who];
            ::close(_fd_array[who]);
            _fd_array[who] = gdefaultsockfd;
        }

            
    }
    void Dispatcher(fd_set &rfds){
        // 遍历辅助数组，查验到底是谁的事件就绪
        for(int i = 0; i < NUM; i++){
            if(_fd_array[i] == gdefaultsockfd) continue;

            // 查验当前这个合法fd，他的位是否在rfds中被内核置为1了？
            if(FD_ISSET(_fd_array[i], &rfds)){
                // 区分事件类型！
                if(_fd_array[i] == _listen_socket->Fd()){
                    // 分支1：门卫亮了说明有新的客户端connect来了！
                    Accepter();
                }else {
                    // 分支2：普通客人亮了，说明是之前建立好连接的客户端发数据过来了！
                    Recver(i);  // 传入i，告诉底层到底是辅助数组里的哪个位置来数据了
                }
            }
        }
    }

    void Loop(){
        fd_set rfds;    // 读文件描述符集：扔给内核的白纸
        _is_running = true;
        
        while(_is_running){
            // 内核每次都会修改rfds，所以每次循环开头，必须拿一张全新的！
            FD_ZERO(&rfds);
            struct timeval timeout = {10, 0};   // 阻塞十秒
            int maxfd = gdefaultsockfd;     // select第一个参数是最大fd值+1，要记录max

            // 重新绘制位图，拿着保存的辅助数组，把有效的fd填充到空白的rfds上
            for(int i = 0; i < NUM; i++){
                if(_fd_array[i] == gdefaultsockfd)
                    continue;   // 无效位跳过

                // 合法的fd，告诉内核：你要帮我盯着合法的fd!
                FD_SET(_fd_array[i], &rfds);

                // select API 强制要求传入所有fd中最大值+1
                // 借着遍历辅助数组的机会，顺手更新最大值
                if(maxfd < _fd_array[i]) maxfd = _fd_array[i];
            }

            // 内核交接：把白纸rfds扔给内核，进程在这里挂起，直到有事件就绪或超时
            int n = ::select(maxfd + 1, &rfds, nullptr, nullptr, &timeout);
            switch(n){
                case 0 : 
                    std::cout << "time out...\n";
                    break;
                case -1 :
                    perror("select");   // 底层信中断等严重错误
                    break;
                default :
                    // 有事件就绪了，内核拿着修改好的rfds回来了
                    std::cout << "有事件就绪了...timeout: " << timeout.tv_sec << ":" << timeout.tv_usec << std::endl;
                    // 把修改后的rfds交给分发器解析
                    Dispatcher(rfds);
                    break;
            }
        }
        _is_running = false;
    }

    ~SelectServer(){}
private:
    uint16_t _port;
    std::unique_ptr<Socket> _listen_socket;     // 监听套接字
    bool _is_running;
    int _fd_array[NUM];      // 辅助数组，记录所有交给select托管的fd
};