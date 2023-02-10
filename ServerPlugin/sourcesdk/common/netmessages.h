
#pragma once

#include "protocol.h"
#include "../public/inetmsghandler.h"
#include "../public/inetmessage.h"
#include "../public/tier1/bitbuf.h"

class IClientMessageHandler;
class INetChannel;
typedef unsigned long long uint64;
class CNetMessageRatelimitPolicyBase;

class CNetMessage : public INetMessage
{
protected:
    CNetMessageRatelimitPolicyBase* m_RateLimitPolicy;
    bool m_bReliable; // true if message should be send reliable
    INetChannel* m_NetChannel; // netchannel this message is from/for
};

class CLC_VoiceData : public CNetMessage
{
public:
    IClientMessageHandler* m_pMessageHandler;
    int m_nLength;
    bf_read m_DataIn;
    bf_write m_DataOut;
    uint64 m_xuid;
};
