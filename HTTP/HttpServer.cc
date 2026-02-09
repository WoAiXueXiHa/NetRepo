#include <iostream>
#include <memory>
#include <string>
#include "HttpServer.hpp"
#include "HttpProtocol.hpp"

// 业务逻辑
// 1. 处理登录
// 只有POST /login并且带参数，才会到这里
void Login(HttpRequest& req, HttpResponse& resp){
    // 1. 拿到参数
    std::string params = req.Args();
    std::cout << "-----------------------" << std::endl;
    std::cout << "正在处理登录业务..." << std::endl;
    std::cout << "用户提交参数: " << params << std::endl;
    std::cout << "-----------------------" << std::endl;

    // 2. 模拟数据库校验 (假装校验成功)
    // 真正的逻辑应该是 Parse 参数，然后去查 MySQL
    
    // 3. 构建动态响应
    // 既然是动态业务，就不读文件了，直接手动设置 Body
    std::string response_body = "<html><body><h1>Login Success! Welcome " + params + "</h1></body></html>";
    
    resp.SetCode(200);
    resp.SetContent(response_body);
    resp.SetHeader("Content-Type", "text/html");
    resp.SetHeader("Content-Length", std::to_string(response_body.size()));
}

// 2. 处理注册
void RegisterUser(HttpRequest &req, HttpResponse &resp)
{
    std::string params = req.Args();
    std::cout << "正在处理注册业务: " << params << std::endl;

    std::string response_body = "<html><body><h1>Register Success! Data: " + params + "</h1></body></html>";
    resp.SetCode(200);
    resp.SetBody(response_body);
    resp.SetHeader("Content-Type", "text/html");
    resp.SetHeader("Content-Length", std::to_string(response_body.size()));
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    // 1. 拿到端口
    uint16_t port = std::stoi(argv[1]);

    // 2. 忽略 SIGPIPE (防止写入已关闭的连接导致崩溃)
    signal(SIGPIPE, SIG_IGN);

    // 3. 创建服务器
    // 使用 unique_ptr 管理，现代 C++ 风格
    std::unique_ptr<HttpServer> server(new HttpServer(port));

    // 4. 注册路由 (Routing)
    // 告诉服务器：如果有人访问 "/login" 且带参数，就调用 Login 函数
    // 注意：这里的 key 要和前端 form 表单里的 action="/login" 对应
    // 还要注意：你之前的逻辑是 _path，如果 URL 是 /login?a=b，path 就是 /login
    server->Register("/login", Login);      // 修正了之前的 typo
    server->Register("/register", RegisterUser); 

    // 5. 启动发动机
    std::cout << "HttpServer running on port " << port << "..." << std::endl;
    server->Start();

    return 0;
}