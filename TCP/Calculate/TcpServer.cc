#include <iostream>
#include <string>
#include <memory>
#include "TcpServer.hpp"
#include "Protocol.hpp"

using namespace ProtocolModule;

// -----------------------------------------------------------
//  业务逻辑
// -----------------------------------------------------------
Response Calculate(const Request& req) {
    Response resp(0, 0);
    switch (req._op) {
        case '+': resp._result = req._x + req._y; break;
        case '-': resp._result = req._x - req._y; break;
        case '*': resp._result = req._x * req._y; break;
        case '/': 
            if (req._y == 0) resp._code = 1;
            else resp._result = req._x / req._y;
            break;
        default: resp._code = 2; break;
    }
    return resp;
}

// ==============================================================
// 核心回调
// ==============================================================
void CalculatorService(int sockfd, InetAddr client){
    std::cout << "[INFO] Start service for " << client.Ip() << ":" << client.Port() << "\n";

    // 缓冲区必须在循环外
    // 缓冲区是蓄水池，用来存包，如果放在里面，每次循环数据就会丢失
    std::string in_buffer;

    while(1){
        // 读数据
        char buffer[1024];
        ssize_t n = ::read(sockfd, buffer, sizeof(buffer) - 1);
        
        if(n > 0){
            buffer[n] = '\0';
            // 把buffer拼接到蓄水池里
            in_buffer += buffer;

            // 双层循环，必须用while循环切割
            // 一次read可能读上来 包1 + 包2 + 半个包3
            // 要连续处理完 包1和包2，把半个包3留下
            while(1){
                std::string payload = Decode(in_buffer);
                if(payload.empty()) break;  // 数据不够一个包，跳出内存循环，去外层循环读

                // 流水线
                // 1> 发送请求
                Request req;
                if (!req.Deserialize(payload)) {
                    std::cerr << "[ERROR] Deserialize failed!" << std::endl;
                    continue; 
                }
                // 2> 调业务回应请求
                Response resp = Calculate(req);
                std::string resp_str;
                resp.Serialize(&resp_str);

                // 3> 封包
                std::string send_packet = Encode(resp_str);

                // 4> 发送
                ::write(sockfd, send_packet.c_str(), send_packet.size()); 
                
                std::cout << "[LOG] Processed one request." << std::endl;
            } 
        } else if (n == 0) {
            std::cout << "[INFO] Client disconnected." << std::endl;
            break;
        }
        else {
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    uint16_t port = std::stoi(argv[1]);
    std::unique_ptr<TcpServer> server(new TcpServer(port));
    server->InitServer();
    server->Loop(CalculatorService); // 注册回调

    return 0;
}
