#include "ws_session.h"
#include "ws_session_mgr.h"

void WsSession::on_read_ws(beast::error_code ec, std::size_t bytes)
{
    if (ec == websocket::error::closed) {
        LOG_ERROR("on_read_ws: {} {}", (int)m_type, m_id);
        WsSessionMgr::printSession();
        WsSessionMgr::leave_session(m_type, m_id);
        LOG_INFO("-----------------");
        WsSessionMgr::printSession();
        return;
    }
    if (ec) {
        return;
    }
    std::string msg = beast::buffers_to_string(m_buffer.data());
    m_buffer.consume(bytes);
    // msg
    do_read();
}