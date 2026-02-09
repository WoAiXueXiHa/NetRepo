#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include "Common.hpp"
#include "Log.hpp"

// HTTP规定的分隔符
const std::string Sep = "\r\n";                 // 每行的结尾
const std::string LineSep = " ";                // 请求行里用空格隔开
const std::string HeaderLineSep = ": ";         // 头部用冒号隔开
const std::string BlankLine = Sep;              // 空行就是单独的\r\n
const std::string HttpVersion = "HTTP/1.0";
const std::string WebRoot = "wwwroot";          // 网页根目录
const std::string FirstPage = "index.html";     // 默认首页文件名
const std::string page404 = "wwwroot/404.html";

// ==========================================
// 类 1: HttpRequest 负责切字符串
// ==========================================
class HttpRequest{
public:
    HttpRequest() :_blank_line(Sep), _isexec(false) {}
    ~HttpRequest() {}

    // 解析请求行
    // 请求行固定为 "METHOD URL VERSION",中间用空格隔开
    void ParseReqLine(std::string& _req_line, const std::string sep){
        (void)sep;
        // 利用stringstream默认以空格为分割符的特性，瞬间完成解析
        std::stringstream ss(_req_line);
        ss >> _method >> _url >> _version;
    }

    // 解析请求头
    bool ParseHeader(std::string& request_str){
        std::string line;
        while(1){
            // 每次拿一行
            bool r = ParseOneLine(request_str, &line, Sep);
            if(r && !line.empty()){
                // 如果这一行有内容，说明是Header，比如 "Host: 127.0.0.1"
                _req_header.push_back(line);
            } else if(r && line.empty()){
                // 如果读到了空行，说明Header结束了
                _blank_line = Sep;
                break;
            } else {
                return false;   // 数据包不完整
            }
            // 把vector里的 "Host: xxx" 拆成键值对
            ParseHeaderkv();
            return true;
        }
    }

    // 解析头部键值对
    bool ParseHeaderkv(){
        std::string key, value;
        for(auto& header : _req_header){
            // 假设header是"Content-Length: 100"
            // HeaderLineSep是": "
            if(SplitString(header, HeaderLineSep, &key, &value)){
                _header_kv.insert(std::make_pair(key, value));
            }
        }
        return true;
    }

    // 反序列化入口：给我一坨字符串，我把它拆成成员变量
    void Deserialize(std::string& request_str){
        // 1. 提取第一行 GET /index.html HTTP/1.1
        if(ParseOneLine(request_str, &_req_line, Sep)){
            // 2. 解析第一行，拆出 Method, URL, Version
            ParseReqLine(_req_line, LineSep);

            // 3. 循环提取Header，只当遇到空行
            ParseHeader(request_str);

            // 4. 剩下所有的内容，都是Body
            _body = request_str;

            // 如果是POST或者GET带参数，需要进一步处理
            if(_method == "POST"){
                _isexec = true;
                _args = _body;          // POST参数在body里！！！
                _path = _url;
            } else if(_method == "GET"){
                // 处理GET /login?user=root&pwd=123
                auto pos = _url.find("?");
                if(-1 != pos){
                    _isexec = true;
                    _path = _url.substr(0, pos);        // login
                    _args = _url.substr(pos + 1);       // user=root&pwd=123
                } else {
                    _path = _url;
                }
            }
        }
    }

    std::string Suffix(){
        // 找最后一个点的位置
        auto pos = _path.rfind(".");
        if (pos == std::string::npos)
            return ".html"; // 没后缀默认当 html
        return _path.substr(pos);
    }

    // get/set->为了外部能获取解析结果
    std::string Args() { return _args; }
    std::string Path() { return _path; }
    std::string Url() { return _url; }
    bool IsHasArgs() { return _isexec; } 

    // 打印调试用
    void Print() {
        std::cout << ">> Method: " << _method << std::endl;
        std::cout << ">> Uri: " << _url << std::endl;
        std::cout << ">> Path: " << _path << std::endl;
        std::cout << ">> Args: " << _args << std::endl;
    }

    std::string GetContent(const std::string path)
    {
        std::string content;
        // [关键点 1]: 打开文件，必须用二进制模式！
        std::ifstream in(path, std::ios::binary);
        
        // [关键点 2]: 容错处理
        if (!in.is_open())
            return std::string(); // 返回空串

        // [关键点 3]: 获取文件大小 (标准三板斧)
        in.seekg(0, in.end);      // 光标移到末尾
        int filesize = in.tellg(); // 获取当前位置(即大小)
        in.seekg(0, in.beg);      // 光标移回开头

        // [关键点 4]: 预分配内存，一次性读入
        content.resize(filesize); 
        in.read((char *)content.c_str(), filesize); // 强转写入
        
        in.close(); // 关闭文件 (其实析构函数会自动关，但显式写也不错)
        
        LOG(DEBUG) << "content length: " << content.size();
        return content;
    }
private: 
    
    // 我们要解析出来的结果
    // 1. 原始数据区
    std::string _req_line;          
    std::vector<std::string> _req_header;
    std::string _blank_line;
    std::string _body;

    // 2. 加工后的数据区
    std::string _method;
    std::string _url;
    std::string _version;
    std::string _path;
    std::string _args;

    std::unordered_map<std::string, std::string> _header_kv;
    bool _isexec;
};

// ==========================================
// 类 2: HttpResponse 负责拼字符串
// ==========================================
class HttpResponse{
public:
    HttpResponse() {}
    ~HttpResponse() {}

    // 向响应中添加一个头部字段
    void SetHeader(const std::string& k, const std::string& v){
        _header_kv[k] = v;
    }
    void SetCode(int code) { _status_code = code; _status_desc = Code2Desc(code); }
    void SetBody(const std::string &body) { _body = body; }
    void SetContent(const std::string &content) { _content = content; }
    
    void Build(HttpRequest& req){
        // 1. 路径处理：拼接本地地址
        std::string url = WebRoot + req.Url();

        // 2. 默认首页处理：如果用户请求的是目录"/"
        if(url.back() == '/') url += FirstPage;

        // 3. 读取资源，尝试从硬盘去读文件
        _content = req.GetContent(url);

        // 4. 状态码判定
        if(_content.empty()){
            _status_code = 404;
            _content = req.GetContent(page404);
        } else {
            _status_code = 200;
        }

        // 5. 翻译状态码
        _status_desc = Code2Desc(_status_code);

        // 6. 设置Content-Length头部
        if(!_content.empty()){
            SetHeader("Content-Length", std::to_string(_content.size()));
        }

        // 7. 设置Content-Type头部
        std::string mime_type = Suffix2Desc(req.Suffix());
        SetHeader("Content-Type", mime_type);

        // 8. 填充正文
        _body = _content;
    }

    void Serialize(std::string* resp_str){
        // 1. 拼接状态行
        _resp_line = _verion + LineSep + std::to_string(_status_code) + 
                     _status_desc + Sep;
    
        // 2. 拼接头部
        for(auto& header : _header_kv){
            _resp_header.push_back(header.first + HeaderLineSep + header.second);
        }

        // 3. 最终组装
        *resp_str = _resp_line;

        for(auto& line : _resp_header){
            *resp_str += (line + Sep);
        }

        // 4. 空行：头部结束标志
        *resp_str += _blank_line;

        // 5. 正文
        *resp_str += _body;
    }

private:
    // 状态码转描述(200->OK 404-> Not Found)
    std::string Code2Desc(int code){
        switch (code){
        case 200:
            return "OK";           // 最常见的成功
        case 404:
            return "Not Found";    // 找不到文件
        case 301:
            return "Moved Permanently"; // [面试点] 永久重定向 (浏览器会缓存这个跳转)
        case 302:
            return "Found";        // [面试点] 临时重定向 (浏览器不会缓存)
        case 500:
            return "Internal Server Error"; // 服务器炸了 (代码逻辑错误)
        default:
            return "Unkown";
        }
    }
    // 文件后缀转MIME类型 (.html->text/html)
    // [P1 逻辑]: 根据文件后缀判断 Content-Type
    std::string Suffix2Desc(const std::string &suffix){
        if (suffix == ".html")
            return "text/html"; // 网页
        else if (suffix == ".css")
            return "text/css";  // 样式表
        else if (suffix == ".js")
            return "application/javascript"; // 脚本
        else if (suffix == ".jpg" || suffix == ".jpeg")
            return "image/jpeg"; 
        else if (suffix == ".png")
            return "image/png";
        else
            return "text/html"; // 默认兜底策略：当网页处理
    }

private:
    // 用来拼接响应的零件
    // 1. 协议版本 (默认 HTTP/1.0)
    std::string _verion; 
    
    // 2. 状态码 (比如 200, 404)
    int _status_code; 
    
    // 3. 状态描述 (比如 "OK", "Not Found")
    std::string _status_desc; 
    
    // 4. 响应头部 (Header) 
    std::unordered_map<std::string, std::string> _header_kv;

    // 5. 响应正文 (Body)
    // _content 是从文件读出来的原始内容
    // _body 是最终要发给网络的正文
    std::string _content;
    std::string _body;

    // 最后拼接出来的字符串
    // 6. 序列化后的结果缓冲
    // 也就是 Serialize 函数跑完后，这些字符串会被填满，准备发给网卡
    std::string _resp_line;          // 第一行
    std::vector<std::string> _resp_header; // 头部列表 (Map 转 Vector 后的结果)
    std::string _blank_line;         // 空行 (\r\n)
};
