#include "rtc/rtc.hpp" // libdatachannel C++ API
#include <opus/opus.h> // libopus
#include <portaudio.h> // PortAudio (blocking mode)
#include <arpa/inet.h> // htonl/htons
#include <random>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

std::vector<uint8_t> buildRtpPacket(const uint8_t *payload, size_t payload_len,
                                    uint16_t seq, uint32_t ts, uint32_t ssrc, uint8_t payloadType)
{
    std::vector<uint8_t> pkt;
    pkt.resize(12 + payload_len);
    // RTP header (12 bytes)
    pkt[0] = 0x80;               // V=2, P=0, X=0, CC=0
    pkt[1] = payloadType & 0x7f; // M=0, PT=payloadType
    uint16_t nseq = htons(seq);
    memcpy(&pkt[2], &nseq, 2);
    uint32_t nts = htonl(ts);
    memcpy(&pkt[4], &nts, 4);
    uint32_t nssrc = htonl(ssrc);
    memcpy(&pkt[8], &nssrc, 4);
    // payload
    memcpy(pkt.data() + 12, payload, payload_len);
    return pkt;
}

// Placeholder: implement these to send SDP/candidates to the peer
void sendSignalingSDPToRemote(const std::string &sdp) { /* your signaling */ }
void sendCandidateToRemote(const std::string &cand, const std::string &mid) { /* your signaling */ }

int main()
{
    // Init logger (optional)
    rtc::InitLogger(rtc::LogLevel::Info, nullptr);

    // 1) PeerConnection config with STUN server (example)
    rtc::Configuration config;
    config.iceServers.emplace_back("stun:stun.l.google.com:19302");

    auto pc = std::make_shared<rtc::PeerConnection>(config);

    // 2) Create audio description and track (use Opus)
    // "audio" is the MID (you can choose), Direction SendOnly or SendRecv
    rtc::Description::Audio media("audio", rtc::Description::Direction::SendOnly);
    const uint8_t preferredPayloadType = 111; // dynamic PT (111 is common for Opus in browsers)
    media.addOpusCodec(preferredPayloadType);
    media.setBitrate(64); // kbps hint (可选)

    auto track = pc->addTrack(media);

    // 3) Signaling callbacks: libdatachannel will call onLocalDescription / onLocalCandidate
    pc->onLocalDescription([&](rtc::Description sdp) {
        // send sdp.str() to remote via your signaling
        sendSignalingSDPToRemote(std::string(sdp));
    });

    pc->onLocalCandidate([&](rtc::Candidate c) {
        sendCandidateToRemote(c.candidate(), c.mid());
    });

    // 4) When track opens (DTLS/SRTP established), start capture->encode->send loop
    track->onOpen([track]() {
        // PortAudio init (blocking) and Opus encoder init
        Pa_Initialize();
        PaStream *stream = nullptr;
        PaStreamParameters inputParams;
        inputParams.device = Pa_GetDefaultInputDevice();
        inputParams.channelCount = 1; // mono
        inputParams.sampleFormat = paInt16;
        inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
        inputParams.hostApiSpecificStreamInfo = nullptr;

        const int sampleRate = 48000; // Opus expects 48kHz
        const int frameSize = 960;    // 20 ms @ 48kHz
        const int maxFrameBytes = 4000;

        Pa_OpenStream(&stream,
                      &inputParams,
                      nullptr,
                      sampleRate,
                      frameSize,
                      paNoFlag,
                      nullptr,
                      nullptr);
        Pa_StartStream(stream);

        int err;
        int opusErr;
        OpusEncoder *enc = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_VOIP, &opusErr);
        opus_encoder_ctl(enc, OPUS_SET_BITRATE(64000));

        // RTP state
        std::random_device rd;
        uint32_t ssrc = rd();
        uint16_t seq = 0;
        uint32_t timestamp = rd(); // random start
        const uint8_t pt = preferredPayloadType;

        std::vector<int16_t> pcm(frameSize);
        std::vector<uint8_t> opus_buf(maxFrameBytes);

        while (true) {
            // blocking read
            Pa_ReadStream(stream, pcm.data(), frameSize);

            // encode
            int nbBytes = opus_encode(enc, pcm.data(), frameSize, opus_buf.data(), maxFrameBytes);
            if (nbBytes < 0)
                continue;

            // build RTP (timestamp increment: frameSize samples)
            auto pkt = buildRtpPacket(opus_buf.data(), nbBytes, seq, timestamp, ssrc, pt);
            // send via libdatachannel track -> libdatachannel will SRTP-protect and send via libjuice/ICE
            track->send(rtc::binary(pkt.begin(), pkt.end()));

            // advance seq/timestamp
            seq++;
            timestamp += frameSize; // 48000Hz * 0.02s = 960
            // small sleep if needed to pace (we already read blocking, so usually not necessary)
        }

        // cleanup (unreachable in this snippet)
        opus_encoder_destroy(enc);
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
    });

    // TODO: exchange SDP/candidates with remote via your signaling. When done, libdatachannel will take care of DTLS/SRTP.
    // Example: createOffer / setLocalDescription is called internally when addTrack + setLocalDescription is invoked in some patterns.
    // For explicit flow, call pc->setLocalDescription() etc as in libdatachannel examples.

    // Keep process alive (in real app, your event loop)
    std::this_thread::sleep_for(std::chrono::hours(24));
    return 0;
}
