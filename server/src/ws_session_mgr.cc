#include "ws_session_mgr.h"
#include "logger.h"

// std::unordered_map<WsSessionId, WsSession::Sptr> WsSessionMgr::s_voip_session;         // <id, session>
// std::unordered_map<WsSessionId, WsSessionStatus> WsSessionMgr::s_voip_session_status;  // <id, status>
// std::unordered_map<WsSessionId, WsSession::Sptr> WsSessionMgr::s_robot_session;        // <id, session>
// std::unordered_map<WsSessionId, WsSessionStatus> WsSessionMgr::s_robot_session_status; // <id, status>
// std::unordered_map<WsSessionId, WsSessionId> WsSessionMgr::s_friend;                   // <id, id>
// std::mutex WsSessionMgr::s_mtx;

bool WsSessionMgr::join_session(WsSessionType type, const WsSessionId &id, WsSession::Sptr ptr)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id) != s_voip_session.end() || s_robot_session.find(id) != s_robot_session.end()) {
        return false;
    }
    bool res;
    switch (type) {
        case kVoip: {
            res = join_voip_session(id, ptr);
            update_voip_session_status(id, kFree);
            break;
        }
        case kRobot: {
            res = join_robot_session(id, ptr);
            update_robot_session_status(id, kFree);
            break;
        }
        default: {
            res = false;
            break;
        }
    }
    return res;
}

bool WsSessionMgr::join_voip_session(const WsSessionId &id, WsSession::Sptr ptr)
{
    if (s_voip_session.find(id) != s_voip_session.end()) {
        return false;
    }
    s_voip_session[id] = ptr;
    match_robot_session(id);
    return true;
}

bool WsSessionMgr::join_robot_session(const WsSessionId &id, WsSession::Sptr ptr)
{
    if (s_robot_session.find(id) != s_robot_session.end()) {
        return false;
    }
    s_robot_session[id] = ptr;
    match_voip_session(id);
    return true;
}

bool WsSessionMgr::leave_session(WsSessionType type, const WsSessionId &id)
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

bool WsSessionMgr::leave_voip_session(const WsSessionId &id)
{
    if (s_voip_session.find(id) == s_voip_session.end()) {
        return false;
    }
    s_voip_session.erase(id);
    if (s_voip2robot.find(id) != s_voip2robot.end()) {
        s_voip2robot.erase(id);
    }
    if (s_robot2voip.find(id) != s_robot2voip.end()) {
        s_robot2voip.erase(id);
    }
    return true;
}

bool WsSessionMgr::leave_robot_session(const WsSessionId &id)
{
    if (s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    s_robot_session.erase(id);
    if (s_voip2robot.find(id) != s_voip2robot.end()) {
        s_voip2robot.erase(id);
    }
    if (s_robot2voip.find(id) != s_robot2voip.end()) {
        s_robot2voip.erase(id);
    }
    return true;
}

bool WsSessionMgr::update_voip_session_status(const WsSessionId &id, WsSessionStatus status)
{
    if (s_voip_session.find(id) == s_voip_session.end()) {
        return false;
    }
    s_voip_session_status[id] = status;
    return true;
}

bool WsSessionMgr::update_robot_session_status(const WsSessionId &id, WsSessionStatus status)
{
    if (s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    s_robot_session_status[id] = status;
    return true;
}

bool WsSessionMgr::relate_session(const WsSessionId &voip_id, const WsSessionId &robot_id)
{
    if (s_voip2robot.find(voip_id) != s_voip2robot.end() || s_robot2voip.find(robot_id) != s_robot2voip.end()) {
        return false;
    }
    s_voip2robot[voip_id] = robot_id;
    s_robot2voip[robot_id] = voip_id;
    return true;
}

WsSession::Sptr WsSessionMgr::get_friend_session(const WsSessionId &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip2robot.find(id) == s_voip2robot.end() && s_robot2voip.find(id) == s_robot2voip.end()) {
        return nullptr;
    }
    WsSession::Sptr friend_ptr;
    if (s_voip2robot.find(id) != s_voip2robot.end()) {
        auto robot_id = s_voip2robot.at(id);
        if (s_robot_session.find(robot_id) != s_robot_session.end()) {
            friend_ptr = s_robot_session.at(robot_id);
        }
    }
    if (s_robot2voip.find(id) != s_robot2voip.end()) {
        auto voip_id = s_robot2voip.at(id);
        if (s_voip_session.find(voip_id) != s_voip_session.end()) {
            friend_ptr = s_voip_session.at(voip_id);
        }
    }
    return friend_ptr;
}

bool WsSessionMgr::match_session(WsSessionType own_type, const WsSessionId &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    if (s_voip_session.find(id) == s_voip_session.end() && s_robot_session.find(id) == s_robot_session.end()) {
        return false;
    }
    bool res;
    switch (own_type) {
        case kVoip: {
            res = match_robot_session(id);
            break;
        }
        case kRobot: {
            res = match_voip_session(id);
            break;
        }
        default: {
            res = false;
            break;
        }
    }
    return res;
}

bool WsSessionMgr::match_voip_session(const WsSessionId &robot_id)
{
    // robot to match voip session
    for (const auto &[voip_id, _] : s_voip_session) {
        if (get_voip_session_status(voip_id) == kFree) {
            relate_session(voip_id, robot_id);
            update_voip_session_status(voip_id, kUsed);
            return true;
        }
    }
    return false;
}

bool WsSessionMgr::match_robot_session(const WsSessionId &voip_id)
{
    // voip to match robot session
    for (const auto &[robot_id, _] : s_robot_session) {
        if (get_robot_session_status(robot_id) == kFree) {
            relate_session(voip_id, robot_id);
            update_robot_session_status(robot_id, kUsed);
            return true;
        }
    }
    return false;
}

WsSessionStatus WsSessionMgr::get_session_status(WsSessionType type, const WsSessionId &id)
{
    std::unique_lock<std::mutex> lock(s_mtx);
    WsSessionStatus status;
    switch (type) {
        case kVoip: {
            status = get_voip_session_status(id);
            break;
        }
        case kRobot: {
            status = get_robot_session_status(id);
            break;
        }
        default: {
            status = kNone;
            break;
        }
    }
    return status;
}

WsSessionStatus WsSessionMgr::get_voip_session_status(const WsSessionId &id)
{
    if (s_voip_session_status.find(id) == s_voip_session_status.end()) {
        return kNone;
    }
    return s_voip_session_status.at(id);
}

WsSessionStatus WsSessionMgr::get_robot_session_status(const WsSessionId &id)
{
    if (s_robot_session_status.find(id) == s_robot_session_status.end()) {
        return kNone;
    }
    return s_robot_session_status.at(id);
}

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

void WsSessionMgr::printFriend()
{
    std::unique_lock<std::mutex> lock(s_mtx);
    LOG_INFO("> friend session");
    for (const auto &[k, v] : s_robot2voip) {
        LOG_INFO("{}: {}", k, v);
    }
    for (const auto &[k, v] : s_voip2robot) {
        LOG_INFO("{}: {}", k, v);
    }
}
