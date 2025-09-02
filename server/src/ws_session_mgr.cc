#include "ws_session_mgr.h"
#include "logger.h"

std::unordered_map<std::string, WsSession::Sptr> WsSessionMgr::s_voip_session;     // <id, session>
std::unordered_map<std::string, std::string> WsSessionMgr::s_voip_session_status;  // <id, status>
std::unordered_map<std::string, WsSession::Sptr> WsSessionMgr::s_robot_session;    // <id, session>
std::unordered_map<std::string, std::string> WsSessionMgr::s_robot_session_status; // <id, status>
std::unordered_map<std::string, std::string> WsSessionMgr::s_friend;               // <id, id>
std::mutex WsSessionMgr::s_mtx;

void WsSessionMgr::printSession()
{
    std::unique_lock<std::mutex> lock(s_mtx);
    LOG_INFO("> voip session");
    for (const auto &[k, v] : s_voip_session) {
        LOG_INFO(" * id: {}, type: {}", k, (int)v->getType());
    }
    LOG_INFO("> robot session");
    for (const auto &[k, v] : s_robot_session) {
        LOG_INFO(" * id: {}, type: {}", k, (int)v->getType());
    }
}

bool WsSessionMgr::join_voip_session(const std::string &id, WsSession::Sptr ptr)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id) != s_voip_session.end()) {
        return false;
    }
    s_voip_session[id] = ptr;
    return true;
}

bool WsSessionMgr::join_robot_session(const std::string &id, WsSession::Sptr ptr)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_robot_session.find(id) != s_robot_session.end()) {
        return false;
    }
    s_robot_session[id] = ptr;
    return true;
}

bool WsSessionMgr::leave_session(WsSessionType type, const std::string &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id) == s_voip_session.end() && s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    switch (type) {
        case kVoip: {
            leave_voip_session(id);
            break;
        }
        case kRobot: {
            leave_robot_session(id);
            break;
        }
        default: {
            break;
        }
    }
    return true;
}

bool WsSessionMgr::leave_voip_session(const std::string &id)
{
    if (s_voip_session.find(id) == s_voip_session.end()) {
        return false;
    }
    s_voip_session.erase(id);
    return true;
}

bool WsSessionMgr::leave_robot_session(const std::string &id)
{
    if (s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    s_robot_session.erase(id);
    return true;
}

bool WsSessionMgr::update_voip_session_status(const std::string &id, WsSessionStatus status)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id) == s_voip_session.end()) {
        return false;
    }
    s_voip_session_status[id] = status;
    return true;
}

bool WsSessionMgr::update_robot_session_status(const std::string &id, WsSessionStatus status)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    s_robot_session_status[id] = status;
    return true;
}

bool WsSessionMgr::relate_session(const std::string &id1, const std::string &id2)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id1) != s_voip_session.end() || s_robot_session.find(id2) != s_robot_session.end()) {
        return false;
    }
    return true;
}

bool WsSessionMgr::match_session(WsSessionType type, const std::string &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    switch (type) {
        case kVoip:
            break;
        case kRobot:
            break;
    }
    return true;
}

bool WsSessionMgr::match_voip_session(const std::string &id)
{
    return true;
}

bool WsSessionMgr::match_robot_session(const std::string &id)
{
    return true;
}