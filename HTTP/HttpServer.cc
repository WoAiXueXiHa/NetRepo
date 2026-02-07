#include <iostream>
#include <memory>
#include "HttpServer.hpp"
#include "Session.hpp"

using namespace std;

// 全局 Session 管理器
SessionManager session_mgr;

// 业务逻辑：登录
void Login(HttpRequest &req, HttpResponse &resp)
{
    // 1. 获取参数
    string args = req.Args(); // username=zhangsan&passwd=123
    LOG(INFO) << "Processing Login: " << args << endl;

    // 2. 模拟验证 (真实项目需解析参数并查数据库)
    // 这里简单粗暴：只要请求了 /login 就当成功
    
    // 3. 创建 Session
    // 生成一个模拟的 SessionID (实际应该用 UUID)
    uint64_t session_id = time(nullptr); 
    session_mgr.CreateSession(session_id, "TeacherCui");

    // 4. 设置响应体
    string html = "<html><body><h1>Login Success!</h1><p>Welcome, TeacherCui</p></body></html>";
    resp.SetContent(html);

    // 5. 关键：种 Cookie！
    // 告诉浏览器："以后来找我，记得带上 JSESSIONID=..."
    string cookie_val = "JSESSIONID=" + to_string(session_id);
    resp.AddHeader("Set-Cookie", cookie_val);
}

// 业务逻辑：注册 (简单的静态页面返回)
void Register(HttpRequest &req, HttpResponse &resp)
{
    string html = "<html><body><h1>Register Page</h1><form action='/login' method='GET'>Name: <input type='text' name='u'><br>Pass: <input type='password' name='p'><br><input type='submit' value='Login'></form></body></html>";
    resp.SetContent(html);
}

// 主函数
int main(int argc, char *argv[])
{
    if(argc != 2) {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }

    // 1. 实例化服务器
    int port = stoi(argv[1]);
    HttpServer server(port);

    // 2. 注册路由 (绑定 URL 和 处理函数)
    // 访问 /login 执行 Login 函数
    server.Register("/login", Login);
    // 访问 /register 执行 Register 函数
    server.Register("/register", Register);

    // 3. 启动服务器
    LOG(INFO) << "HttpServer Starting on port " << port << "...";
    server.Start();

    return 0;
}