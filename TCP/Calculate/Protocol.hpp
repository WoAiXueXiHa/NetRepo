#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <jsoncpp/json/json.h>

namespace ProtocolModule{
    // =============================================
    // 第一层：解决TCP粘包问题
    // ---------------------------------------------
    // TCP面向字节流
    // 像是自来水厂直接一次性给你提供很多水
    // 你作为用户，可以接一杯，可以接一桶
    // 所以我们需要人为规定：多长才算一个完整的报文？
    // 格式规定为："长度\r\nJSON字符串\r\n"
    // =============================================

    //  封包：给数据加报头
    // 输入: "{"x":10...}"
    // 输出: "15\r\n{"x":10...}\r\n"
    std::string Encode(const std::string& json_str){
        std::string len_str = std::to_string(json_str.size());
        std::string packet = len_str + "\r\n" + json_str + "\r\n";
        return packet;
    }

    // 解包：从缓冲区里提取一个完整的报文
    // 返回值: 提取出的 JSON 字符串 (如果没有完整的包，返回空字符串)
    std::string Decode(std::string &buffer) {
        // 1. 找报头分隔符 "\r\n"
        size_t pos = buffer.find("\r\n");
        if (pos == std::string::npos) return ""; // 说明连长度都没收全，继续等

        // 2. 提取长度
        std::string len_str = buffer.substr(0, pos);
        int len = std::stoi(len_str);

        // 3. 计算本包总长度 = 长度字符串 + 2字节分隔符 + 数据长度 + 2字节分隔符
        int total_len = len_str.size() + 2 + len + 2;

        // 4. 检查缓冲区剩下的数据够不够一个包
        if (buffer.size() < total_len) return ""; // 数据不够，继续等

        // 5. 提取数据 (切掉报头和报尾)
        std::string json_str = buffer.substr(pos + 2, len);

        // 6. 从缓冲区移除这个已经提取的包
        buffer.erase(0, total_len);

        return json_str;
    }

    // ================================================================
    // 第二层：序列化与反序列化 (Serialize / Deserialize)
    // ----------------------------------------------------------------
    // 作用：把 C++ 的 struct 对象 <---> 字符串
    // 这里我们用 JSON 库来做这个转换
    // ================================================================

    // 请求结构体：客户端发给服务器的
    class Request {
    public:
        int _x;
        int _y;
        char _op; // '+', '-', '*', '/'

        Request() : _x(0), _y(0), _op(0) {}
        Request(int x, int y, char op) : _x(x), _y(y), _op(op) {}

        // 序列化: struct -> string
        bool Serialize(std::string *out) {
            Json::Value root;
            root["x"] = _x;
            root["y"] = _y;
            root["op"] = _op; // char 会被转成 ASCII 整数存储，问题不大

            // FastWriter 负责把对象转成字符串
            Json::FastWriter writer;
            *out = writer.write(root);
            return true;
        }

        // 反序列化: string -> struct
        bool Deserialize(const std::string &in) {
            Json::Value root;
            Json::Reader reader;
            // parse 负责把字符串解析回对象
            bool res = reader.parse(in, root);
            if (!res) return false;

            _x = root["x"].asInt();
            _y = root["y"].asInt();
            _op = (char)root["op"].asInt();
            return true;
        }
    };

    // 响应结构体：服务器回给客户端的
    class Response {
    public:
        int _result;
        int _code; // 0:成功, 1:除0错误, 2:非法操作

        Response() : _result(0), _code(0) {}
        Response(int res, int code) : _result(res), _code(code) {}

        // 序列化
        bool Serialize(std::string *out) {
            Json::Value root;
            root["result"] = _result;
            root["code"] = _code;

            Json::FastWriter writer;
            *out = writer.write(root);
            return true;
        }

        // 反序列化
        bool Deserialize(const std::string &in) {
            Json::Value root;
            Json::Reader reader;
            bool res = reader.parse(in, root);
            if (!res) return false;

            _result = root["result"].asInt();
            _code = root["code"].asInt();
            return true;
        }
    };
}