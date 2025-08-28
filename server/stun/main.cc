// juice_jrtp_demo.cpp
// Build example:
// g++ juice_jrtp_demo.cpp -o juice_jrtp_demo `pkg-config --cflags --libs` -ljuice -ljrtplib -pthread
//
// Note: adjust pkg-config include flags as needed. Ensure juice/juice.h and jrtplib headers are installed.

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>

extern "C" {
#include <juice/juice.h>
}

#include "jrtplib3/rtpsession.h"
#include "jrtplib3/rtpudpv4transmitter.h"
#include "jrtplib3/rtpipv4address.h"
#include "jrtplib3/rtpsessionparams.h"
#include "jrtplib3/rtperrors.h"

using namespace jrtplib;

static juice_agent_t *agent = nullptr;

static void print_error_and_exit(const char *msg)
{
    std::cerr << msg << std::endl;
    if (agent)
        juice_destroy(agent);
    exit(1);
}

int main()
{
    // 1) 配置 libjuice agent
    juice_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    // 可选：设置 STUN 服务器（方便公网映射发现）：
    cfg.stun_server_host = const_cast<char *>("stun.l.google.com");
    cfg.stun_server_port = 19302;
    // cfg.turn_servers = ... // 若需要 TURN，按 libjuice README 填写
    // 回调可以置空（demo 将轮询状态），但你也可以设置 cb_recv/cb_state_changed 等
    cfg.user_ptr = nullptr;
    cfg.cb_state_changed = nullptr;
    cfg.cb_candidate = nullptr;
    cfg.cb_gathering_done = nullptr;
    cfg.cb_recv = nullptr;

    agent = juice_create(&cfg);
    if (!agent)
        print_error_and_exit("juice_create failed");

    std::cout << "[libjuice] agent created\n";

    // 2) 开始收集候选
    if (juice_gather_candidates(agent) != JUICE_ERR_SUCCESS) {
        print_error_and_exit("juice_gather_candidates failed");
    }

    // 小等一下让初次 gather 触发（生产不 rely on sleep）
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // 3) 获取本地 description（SDP-like），打印以便通过信令发给 peer
    char sdp[JUICE_MAX_SDP_STRING_LEN] = {0};
    if (juice_get_local_description(agent, sdp, sizeof(sdp)) != JUICE_ERR_SUCCESS) {
        print_error_and_exit("juice_get_local_description failed");
    }
    std::cout << "=== LOCAL DESCRIPTION (send this to remote peer) ===\n";
    std::cout << sdp << "\n";
    std::cout << "=== END ===\n\n";

    // 4) 等候用户把 remote description paste 回来（在真实应用这一步通过信令自动）
    std::cout << "Paste remote description (end with a single line containing only END), then press Enter:\n";
    std::string remote_sdp;
    std::string line;
    while (true) {
        if (!std::getline(std::cin, line))
            break;
        if (line == "END")
            break;
        remote_sdp += line + "\n";
    }
    if (remote_sdp.empty()) {
        print_error_and_exit("No remote description provided");
    }

    // 5) 设置远端 description
    if (juice_set_remote_description(agent, remote_sdp.c_str()) != JUICE_ERR_SUCCESS) {
        print_error_and_exit("juice_set_remote_description failed");
    }
    std::cout << "[libjuice] remote description set\n";

    // 6) 再次确保 candidates 正在收集（如果你用 trickle ICE，可在这里处理）
    // juice_gather_candidates(agent); // optional depending on your signaling/trickle design

    // 7) 等待 ICE 完成（轮询状态，或在真实项目用 cb_state_changed 回调）
    std::cout << "[libjuice] waiting for ICE to complete ...\n";
    for (int i = 0; i < 200; ++i) { // 超时约 20s
        juice_state_t state = juice_get_state(agent);
        if (state == JUICE_STATE_COMPLETED || state == JUICE_STATE_CONNECTED) {
            std::cout << "[libjuice] ICE completed/connected\n";
            break;
        }
        else if (state == JUICE_STATE_FAILED || state == JUICE_STATE_DISCONNECTED) {
            print_error_and_exit("ICE failed/disconnected");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 8) 取得选定地址（local / remote）
    char local_ip[JUICE_MAX_ADDRESS_STRING_LEN] = {0};
    char remote_ip[JUICE_MAX_ADDRESS_STRING_LEN] = {0};
    int local_port = 0, remote_port = 0;
    if (juice_get_selected_addresses(agent,
                                     local_ip, sizeof(local_ip),
                                     remote_ip, sizeof(remote_ip)) != JUICE_ERR_SUCCESS) {
        print_error_and_exit("juice_get_selected_addresses failed");
    }

    std::cout << "[libjuice] selected addresses:\n";
    std::cout << "  local  = " << local_ip << ":" << local_port << "\n";
    std::cout << "  remote = " << remote_ip << ":" << remote_port << "\n";

    // 9) 用 jrtplib 绑定本地端口并向 remote 推 RTP
    RTPSession sess;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;

    // 设置时基（示例 8kHz 音频），以及其它参数按需调整
    sessparams.SetOwnTimestampUnit(1.0 / 8000.0);
    sessparams.SetAcceptOwnPackets(true);

    // 注意：jrtplib 的 SetPortbase 只设置 RTP 端口，本例假设 remote_port 对应 RTP。
    transparams.SetPortbase((uint16_t)local_port);

    int status = sess.Create(sessparams, &transparams);
    if (status < 0) {
        std::cerr << "jrtplib Create error: " << RTPGetErrorString(status) << std::endl;
        juice_destroy(agent);
        return 1;
    }

    // 转换 remote IP 为网络序
    in_addr addr;
    if (inet_aton(remote_ip, &addr) == 0) {
        std::cerr << "Invalid remote IP: " << remote_ip << std::endl;
        sess.BYEDestroy(RTPTime(0, 0), NULL, 0);
        juice_destroy(agent);
        return 1;
    }
    uint32_t ip_n = ntohl(addr.s_addr);
    RTPIPv4Address dest((uint8_t *)&ip_n, (uint16_t)remote_port);

    status = sess.AddDestination(dest);
    if (status < 0) {
        std::cerr << "AddDestination error: " << RTPGetErrorString(status) << std::endl;
        sess.BYEDestroy(RTPTime(0, 0), NULL, 0);
        juice_destroy(agent);
        return 1;
    }

    std::cout << "[jrtplib] started. Sending 20 RTP packets to " << remote_ip << ":" << remote_port << "\n";

    // 10) 发送 RTP 包（示例 payload），真实场景把编码后音频帧塞入
    for (int i = 0; i < 20; ++i) {
        const char payload[] = "libjuice+jrtplib demo payload";
        int send_status = sess.SendPacket((void *)payload, (int)strlen(payload), /*pt=*/96, /*mark=*/false, /*timestamp_inc=*/160);
        if (send_status < 0) {
            std::cerr << "SendPacket error: " << RTPGetErrorString(send_status) << std::endl;
            break;
        }
        RTPTime::Wait(RTPTime(0, 200000)); // 200ms
    }

    sess.BYEDestroy(RTPTime(3, 0), NULL, 0);
    std::cout << "[jrtplib] finished sending, session closed\n";

    // 11) cleanup libjuice
    juice_destroy(agent);
    agent = nullptr;

    return 0;
}
