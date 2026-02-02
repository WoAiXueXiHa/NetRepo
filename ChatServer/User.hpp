#pragma once
#include <list>
#include <algorithm>
#include "InetAddr.hpp"
#include "Log.hpp"

using namespace LogModule;
 
// ------------------------------------------------
// 抽象接口：UserInterface
// 为了后续扩展TCP用户、WebSocket用户，定义通用行为
// ------------------------------------------------
class UserInterface{
public:
    virtual ~UserInterface() = default;
    virtual void SendTo(int sockfd, const std::string& msg) = 0;
    virtual std::string Id() = 0;
    virtual bool operator==(const InetAddr& u) = 0;
};

class User : public UserInterface{
public:
    User(const InetAddr& id) :_id(id) {}
    // 发消息
    void SendTo(int sockfd, const std::string& msg) override{
        // 调用系统API发送UDP报文
        // UDP时无连接的，每次发送都要指定对方的地址
        ::sendto(sockfd, msg.c_str(), msg.size(), 0, _id.NetAddr(), _id.NetAddrLen());
    }
    bool operator==(const InetAddr& u) override { return _id == u; }
    std::string Id() override { return _id.Addr(); }
private:
    InetAddr _id;       // IP + Port
};

// ------------------------------------------------
// 管理类：UserManager
// 维护在线用户列表，负责广播消息
// ------------------------------------------------
class UserManager{
public:
    // 新增用户
    void AddUser(InetAddr& id){
        LockGuard lock(_mutex);
        // 遍历列表去重
        for(auto& user: _users){
            if(*user == id) return;     //  已经在列表里，不用操作
        }
        _users.push_back(std::make_shared<User>(id));
        LOG(INFO) << "新用户上线：" << id.Addr();
    }

    // 删除用户
    void DelUser(InetAddr& id){
        LockGuard lock(_mutex);
        _users.remove_if([&id](std::shared_ptr<UserInterface>& u){
            return *u == id;
        });
        LOG(INFO) << "用户下线：" << id.Addr();
    }

    // 群发
    void Router(int sockfd, const std::string& msg){
        LockGuard lock(_mutex);
        for(auto& user: _users){
            user->SendTo(sockfd, msg);
        }
    }
private:
    std::list<std::shared_ptr<UserInterface>> _users;
    LockModule::Mutex _mutex;
};