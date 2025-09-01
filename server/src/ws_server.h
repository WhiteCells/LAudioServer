#ifndef _WS_SERVER_H_
#define _WS_SERVER_H_

#include "ws_session.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <string>
#include <mutex>
#include <unordered_map>

namespace net = boost::asio;
using tcp = net::ip::tcp;

class WsServer
{
private:
    tcp::acceptor m_acceptor;
    tcp::endpoint m_endpoint;
    std::mutex m_session_mtx;

    // voip 类型 session
    std::unordered_map<std::string, WsSession::Sptr> m_voip_session;    // <id, session>
    std::unordered_map<std::string, std::string> m_voip_session_status; // <id, status>

    // robot 类型 session
    std::unordered_map<std::string, WsSession::Sptr> m_robot_session;    // <id, session>
    std::unordered_map<std::string, std::string> m_robot_session_status; // <id, status>

    // voip 和 robot 配对
    std::unordered_map<std::string, std::string> m_friend; // <id, id>

public:
    WsServer(net::io_context &ioc,
             const std::string &addr,
             const unsigned short port);
    ~WsServer();

    void send(const std::string &id,
              const std::string &msg);

private:
    void do_accept();
};

#endif // _WS_SERVER_H_