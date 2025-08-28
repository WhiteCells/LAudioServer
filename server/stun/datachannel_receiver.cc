// receiver.cpp (示例，重点展示如何接收并解码)
#include "rtc/rtc.hpp"
#include <opus/opus.h>
#include <portaudio.h>
#include <iostream>

// Placeholder: implement signaling to receive offer from remote and send answer
std::string waitForRemoteOffer() { /* your signaling */ return ""; }
void sendAnswerToRemote(const std::string &sdp) { /* your signaling */ }

int main() {
    rtc::InitLogger(rtc::LogLevel::Info, nullptr);
    rtc::Configuration config;
    config.iceServers.emplace_back("stun:stun.l.google.com:19302");
    auto pc = std::make_shared<rtc::PeerConnection>(config);

    // When a remote track arrives:
    pc->onTrack([&](std::shared_ptr<rtc::Track> track) {
        // set a depacketizer / or onFrame callback
        // The library offers Opus depacketizer helper in examples; we can use onFrame
        // frame payload will be raw RTP payload (after depacketizing if you set a MediaHandler)
        track->onFrame([&](rtc::binary data, rtc::FrameInfo info) {
            // data is a full payload as provided by media handler (opus payload)
            static OpusDecoder* dec = nullptr;
            if (!dec) {
                int err;
                dec = opus_decoder_create(48000, 1, &err);
                // init PortAudio playback
                Pa_Initialize();
            }
            // decode
            const uint8_t* payload = data.data();
            int payload_len = (int)data.size();
            const int frameSize = 960;
            std::vector<int16_t> pcm(frameSize);
            int samples = opus_decode(dec, payload, payload_len, pcm.data(), frameSize, 0);
            if (samples > 0) {
                // play via PortAudio blocking write (setup stream earlier in full code)
                // Pa_WriteStream(playStream, pcm.data(), samples);
            }
        });
    });

    // Accept offer from remote and answer
    std::string offer = waitForRemoteOffer();
    if (!offer.empty()) {
        pc->setRemoteDescription(rtc::Description(offer));
        // create answer, onLocalDescription callback will be fired and you send via signaling
    }

    // keep alive...
    std::this_thread::sleep_for(std::chrono::hours(24));
}
