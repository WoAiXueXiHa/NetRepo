#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>

class Session
{
public:
    Session(std::string u, uint64_t id) : username(u), session_id(id) {}
    std::string username;
    uint64_t session_id;
};

// 使用智能指针管理 Session，防止内存泄漏
using SessionPtr = std::shared_ptr<Session>;

// ==========================================
// SessionManager：单例或全局管理类
// ==========================================
class SessionManager
{
public:
    // 场景：用户登录成功后调用
    // 必须理解：为什么用 shared_ptr？(方便生命周期管理)
    void CreateSession(uint64_t session_id, std::string user)
    {
        _sessions[session_id] = std::make_shared<Session>(user, session_id);
    }

    // 场景：用户每次请求带 Cookie 来时调用
    // 逻辑：拿着 ID 去 map 里查，查到了说明已登录
    SessionPtr SearchSession(uint64_t session_id)
    {
        if (_sessions.find(session_id) != _sessions.end()) {
            return _sessions[session_id];
        }
        return nullptr;
    }
    
    void DeleteSession(uint64_t session_id)
    {
        _sessions.erase(session_id);
    }

private:
    // 哈希表：Key是SessionID，Value是Session对象指针
    std::unordered_map<uint64_t, SessionPtr> _sessions;
};