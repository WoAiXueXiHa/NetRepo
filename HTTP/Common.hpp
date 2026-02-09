#pragma once
#include <iostream>
#include <string>
#include <cstdlib>

enum {
    USAGE_ERR = 1,
    SOCKET_ERR,
    BIND_ERR,
    LISTEN_ERR
};
#define Conv(v) (struct sockaddr*)(v)
const int gdefaultsockfd = -1;
const int gbacklog = 8;

// 核心工具：按分隔符切分字符串
// 场景：HTTP协议规定每行以"\r\n"结尾，需要一行一行解析
bool ParseOneLine(std::string& str, std::string* out, const std::string& sep){
    // find返回的是下表，找不到返回-1
    auto pos = str.find(sep);
    if(-1 == pos) return false;     // 说明这不是完整的一行，可能数据包还没收全
    *out = str.substr(0, pos);

    // 一行解析完，删除这一行，继续下一行，所以str是引用，直接操作本体
    str.erase(0, pos + sep.size());
    return true;
}

// 场景：把 "Connection: keep-alive" 切割成键值对 "Connection" 和 "keep-alive"
bool SplitString(const std::string& header, const std::string& sep, 
                 std::string* key, std::string* value){
    auto pos = header.find(sep);
    if(-1 == pos) return false;

    *key = header.substr(0, pos);
    *value = header.substr(pos + sep.size());
    return true;
}