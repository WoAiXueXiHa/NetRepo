#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "InetAddr.hpp"

// 定义常见的 HTTP 工具函数
const std::string Sep = "\r\n";
const std::string WebRoot = "./wwwroot"; // 网站根目录
const std::string HomePage = "index.html"; // 默认首页

// HTTP请求类，负责把字符串编程对象
class HttpRequest{
public:
    HttpRequest() : _blank_line(Sep), _is_args(false) {}

    // 反序列化
    void ParseRequestLine(const std::string &line)
    {
        std::stringstream ss(line);
        ss >> _method >> _url >> _version;
        if (_url == "/") _url = "/" + HomePage; 
    }
    void Deserialize(std::string req){
        // 1. 提取第一行，请求行
        // 找到第一个\r\n，切割
        size_t pos = req.find(Sep);
        if(pos == std::string::npos) return;
        std::string req_line = req.substr(0, pos);

        // 2. 解析请求行
        ParseRequestLine(req_line);

        // 3. 提取body
        size_t body_pos = req.find(Sep + Sep);
        if(body_pos != std::string::npos){
            _body = req.substr(body_pos + 2 * Sep.size());
        }

        // GET在URL的?后面
        // POST在body里
        if(_method == "POST" && !_body.empty()){
            _is_args = true;
            _args = _body;
        } else if(_method == "GET" && _url.find('?') != std::string::npos){
            _is_args = true;
            size_t q_pos = _url.find('?');
            _path = _url.substr(0, q_pos);  // '?'前面是路径 /login
            _args = _url.substr(q_pos + 1); // '?'后面是参数 u=1&p=2
        } else {
            _path = _url;   // 没有参数，路径就是URL本身
        }
    }

     std::string GetContent(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) {
            // 打开失败打印个日志，方便排查
            // LOG(LogLevel::ERROR) << "Open file failed: " << path; 
            return "";
        }

        // 1. 获取文件长度
        in.seekg(0, in.end);
        int filesize = in.tellg();
        in.seekg(0, in.beg);

        std::string content;
        // 2. 预分配空间 (这一步很重要，否则 size 是 0)
        content.resize(filesize);

        // 3. 读取文件 (注意：这里要用 &content[0]，不能用 c_str())
        in.read(&content[0], filesize);
        
        in.close();
        return content;
    }

    bool IsHasArgs() { return _is_args; }
    std::string Path() { return _path; }
    std::string Args() { return _args; }
    std::string Method() { return _method; }
    std::string Body() { return _body; }
    
    std::string GetHeader(const std::string &key) {
        if (_headers.find(key) != _headers.end()) return _headers[key];
        return "";
    }
private:
    std::string _method;
    std::string _url;
    std::string _path;
    std::string _args;
    std::string _version;
    std::string _body;
    std::string _blank_line;
    bool _is_args;
    std::unordered_map<std::string, std::string> _headers;
};

// HTTP应答类，把对象编程字符串
class HttpResponse{
public:
    HttpResponse() : _version("HTTP/1.0"), _code(200), _desc("OK") {}

    // 构建静态资源响应
    void Build(HttpRequest& req){
        std::string path = WebRoot + req.Path();
        if(path.back() == '/') path += HomePage;

        struct stat st;
        if(stat(path.c_str(), &st) < 0){
            _code = 404;
            _desc = "Not Found";
            path = WebRoot + "/404.html";
            stat(path.c_str(), &st);
        } else {
            _code = 200;
            _desc = "OK";
        }

        int fd = ::open(path.c_str(), O_RDONLY);
        if(fd >= 0){
            // 1. 必须用 resize 开辟实际空间，这样 _body.size() 才会变大
            _body.resize(st.st_size); 
            // 2. 使用 &_body[0] 获取可写内存地址
            read(fd, &_body[0], st.st_size);
            ::close(fd);
        }

        AddHeader("Content-Length", std::to_string(_body.size()));
        AddHeader("Content-Type", GetContentType(path));
    }

    void SetContent(const std::string &content, const std::string &type = "text/html") {
        _body = content;
        AddHeader("Content-Length", std::to_string(_body.size()));
        AddHeader("Content-Type", type);
    }

    void Serialize(std::string *output)
    {
        // 状态行
        *output = _version + " " + std::to_string(_code) + " " + _desc + Sep;
        // 响应头
        for (auto &h : _headers) {
            *output += h.first + ": " + h.second + Sep;
        }
        // 空行
        *output += Sep;
        // 正文
        *output += _body;
    }
    
    void AddHeader(const std::string &k, const std::string &v) {
        _headers[k] = v;
    }

private:
    std::string GetContentType(const std::string &path) {
        if (path.find(".html") != std::string::npos) return "text/html";
        if (path.find(".jpg") != std::string::npos) return "image/jpeg";
        return "text/plain";
    }

    std::string _version;
    int _code;
    std::string _desc;
    std::unordered_map<std::string, std::string> _headers;
    std::string _body;
};