#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include "Common.hpp"

// HTTP规定的分隔符
const std::string Sep = "\r\n";                 // 每行的结尾
const std::string LineSep = " ";                // 请求行里用空格隔开
const std::string HeaderLineSep = ": ";         // 头部用冒号隔开
const std::string BlankLine = Sep;              // 空行就是单独的\r\n
const std::string HttpVersion = "HTTP/1.0";
const std::string WebRoot = "wwwroot";          // 网页根目录

// ==========================================
// 类 1: HttpRequest (负责切字符串)
// ==========================================
class HttpRequest{
public:
    HttpRequest() :_blank_line(Sep), _isexec(false) {}
    ~HttpRequest() {}

    // 反序列化入口：给我一坨字符串，我把它拆成成员变量
    void Deserialize(std::string& request_str);

    // get/set->为了外部能获取解析结果
    std::string Path() { return _path; }
    std::string Url() { return _url; }
    bool ISHasArgs() { return _isexec; } 

    // 打印调试用
    void Print() {
        std::cout << ">> Method: " << _method << std::endl;
        std::cout << ">> Uri: " << _url << std::endl;
        std::cout << ">> Path: " << _path << std::endl;
        std::cout << ">> Args: " << _args << std::endl;
    }
private: 
    // 解析请求行
    void ParseReqLine(std::string& line);
    // 解析头部
    void ParseHeader(std::vector<std::string>& headers);
public:
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
private:
    // 状态码转描述(200->OK 404-> Not Found)
    std::string Code2Desc(int code);
    // 文件后缀转MIME类型 (.html->text/html)
    std::string Suffix2Desc(const std::string& suffix);

private:
    // 用来拼接响应的零件
    std::string _version;
    int _status_code;
    std::string _status_desc;

    std::unordered_map<std::string, std::string> _header_kv;
    std::string _body;

    // 最后拼接出来的字符串
    std::string _resp_line;
    std::vector<std::string> _resp_header;
    std::string _blank_line;
};
