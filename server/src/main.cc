#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtptransmitter.h>
#include <jrtplib3/rtpipv4address.h>

class RTPS : public jrtplib::RTPSession
{
    virtual void OnRTPPacket(jrtplib::RTPPacket *pack, const jrtplib::RTPTime &receivetime, const jrtplib::RTPAddress *senderaddress) override
    {
        // senderaddress ip + port
        m_session->AddDestination(*senderaddress);
        auto ipv4 = (jrtplib::RTPIPv4Address *)senderaddress;
        ipv4->GetPort();
    }

private:
    jrtplib::RTPSession *m_session;
};

int main(int argc, char *argv[])
{

    return 0;
}