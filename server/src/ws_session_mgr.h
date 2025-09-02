#ifndef _WS_SESSION_MGR_H_
#define _WS_SESSION_MGR_H_

#include "types.h"
#include "ws_session.h"
#include <string>
#include <mutex>

class WsSessionMgr
{
public:
    WsSessionMgr() = default;
    ~WsSessionMgr() = default;

    static void printSession();

    static bool join_voip_session(const std::string &id, WsSession::Sptr ptr);
    static bool join_robot_session(const std::string &id, WsSession::Sptr ptr);

    static bool leave_session(WsSessionType type, const std::string &id);
    static bool leave_voip_session(const std::string &id);
    static bool leave_robot_session(const std::string &id);

    static bool update_voip_session_status(const std::string &id, WsSessionStatus status);
    static bool update_robot_session_status(const std::string &id, WsSessionStatus status);

    static bool relate_session(const std::string &id1, const std::string &id2);

    static bool match_session(WsSessionType type, const std::string &id);
    static bool match_voip_session(const std::string &id);
    static bool match_robot_session(const std::string &id);

private:
    // voip 类型 session
    static std::unordered_map<std::string, WsSession::Sptr> s_voip_session;    // <id, session>
    static std::unordered_map<std::string, std::string> s_voip_session_status; // <id, status>
    // robot 类型 session
    static std::unordered_map<std::string, WsSession::Sptr> s_robot_session;    // <id, session>
    static std::unordered_map<std::string, std::string> s_robot_session_status; // <id, status>
    // voip 和 robot 配对
    static std::unordered_map<std::string, std::string> s_friend; // <id, id>

    static std::mutex s_mtx;
};

#endif // _WS_SESSION_MGR_H_