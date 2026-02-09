#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include "TcpServer.hpp"
#include "HttpProtocol.hpp"
#include "Log.hpp"

using namespace TcpServerModule;

using http_handler_t = std::function<void(HttpRequest &, HttpResponse &)>;

class HttpServer
{
public:
    HttpServer(int port)
        : _tsvr(std::make_unique<TcpServer>(port))
    {
    }

    // 注册路由
    // 作用：往 map 里添加 URL->函数的映射
    void Register(std::string funcname, http_handler_t func)
    {
        _route[funcname] = func;
    }

    // 启动服务器
    // 作用：把 HandlerHttpRequest 绑定给 TcpServer 的回调
    void Start()
    {
        _tsvr->InitServer([this](SockPtr sockfd, InetAddr client)
                          { return this->HandlerHttpRequest(sockfd, client); });
        _tsvr->Loop();
    }

    // Recv -> Deserialize -> 路由判断 -> (Callback 或 Build) -> Serialize -> Send
    bool HandlerHttpRequest(SockPtr sockfd, InetAddr client)
    {
        // 1. 读取 TCP 数据
        std::string http_request;
        int n = sockfd->Recv(&http_request); 
        if (n <= 0) return false;

        // 2. 反序列化 (String -> Request对象)
        HttpRequest req;
        req.Deserialize(http_request);

        HttpResponse resp;
        
        // 问：静态网页和 API 接口怎么区分处理？
        // 答：先看有没有参数，再看路由表里有没有注册这个 URL
        if (req.IsHasArgs() || _route.find(req.Path()) != _route.end())
        {
            if (_route.find(req.Path()) != _route.end()) {
                // 动态逻辑：命中路由表，调用注册的业务函数 (如 Login)
                _route[req.Path()](req, resp); 
            } else {
                // 虽然有参数但没路由，还是当静态资源兜底处理
                resp.Build(req); 
            }
        }
        else
        {
            // 静态资源：直接读文件
            resp.Build(req);
        }

        // 4. 序列化并发送 (Response对象 -> String -> TCP Send)
        std::string resp_str;
        resp.Serialize(&resp_str);
        sockfd->Send(resp_str);
        return true;
    }

    ~HttpServer() {}

private:
    std::unique_ptr<TcpServer> _tsvr;
    // 这里的 Key 是 URL 路径 (如 "/login")，Value 是对应的函数
    std::unordered_map<std::string, http_handler_t> _route; 
};