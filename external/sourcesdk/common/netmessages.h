
#pragma once

#include "../public/inetchannelinfo.h"
#include "../public/inetmsghandler.h"
#include "../public/inetmessage.h"
#include "../public/tier1/bitbuf.h"

class IClientMessageHandler;
class IServerMessageHandler;
class INetChannel;
typedef unsigned long long uint64;
class CNetMessageRatelimitPolicyBase;

class CNetMessage : public INetMessage
{
public:
#ifndef SDK_COMPAT
    CNetMessageRatelimitPolicyBase* m_RateLimitPolicy;
#endif
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

class SVC_VoiceData : public CNetMessage
{
public:
    SVC_VoiceData();
    ~SVC_VoiceData();

    virtual void Destructor() override;

    virtual void SetNetChannel(INetChannel* netchan) override;
    virtual void SetReliable(bool state) override;

    virtual bool Process() override;

    virtual bool ReadFromBuffer(bf_read& buffer) override;
    virtual bool WriteToBuffer(bf_write& buffer) override;

    virtual bool IsReliable() const override;

    virtual int GetType() const override;
    virtual int GetGroup() const override;
    virtual const char* GetName() const override;
    INetChannel* GetNetChannel() const override;
    virtual const char* ToString() const override;

    virtual bool BIncomingMessageForProcessing(double, int) const override;
    virtual int GetSize() const override;
    virtual void SetRatePolicy() override;

public:
    IServerMessageHandler* m_pMessageHandler;

    int m_nFromClient;  // client who has spoken
    bool m_bProximity;
    int m_nLength;  // data length in bits
    uint64 m_xuid;  // X360 player ID

    bf_read m_DataIn;
    void* m_DataOut;
};

class SVC_SetView : public CNetMessage
{
public:
    virtual void Destructor() override;

    virtual void SetNetChannel(INetChannel* netchan) override;
    virtual void SetReliable(bool state) override;

    virtual bool Process() override;

    virtual bool ReadFromBuffer(bf_read& buffer) override;
    virtual bool WriteToBuffer(bf_write& buffer) override;

    virtual bool IsReliable() const override;

    virtual int GetType() const override;
    virtual int GetGroup() const override;
    virtual const char* GetName() const override;
    INetChannel* GetNetChannel() const override;
    virtual const char* ToString() const override;

    virtual bool BIncomingMessageForProcessing(double, int) const override;
    virtual int GetSize() const override;
    virtual void SetRatePolicy() override;

public:
    int m_nEntityIndex = 0;
};
