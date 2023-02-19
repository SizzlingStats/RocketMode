
#include "netmessages.h"
#include "protocol.h"
#include "../engine/net.h"

// Only need to implement WriteToBuffer, IsReliable, and constructor.
// Can't get around this unless I get a handle to an allocated SVC_VoiceData.
SVC_VoiceData::SVC_VoiceData() :
    m_pMessageHandler(nullptr),
    m_nFromClient(0),
    m_bProximity(false),
    m_nLength(0),
    m_xuid(0),
    m_DataIn(),
    m_DataOut(nullptr)
{
}

SVC_VoiceData::~SVC_VoiceData()
{
}

void SVC_VoiceData::Destructor()
{
}

void SVC_VoiceData::SetNetChannel(INetChannel* netchan)
{
}

void SVC_VoiceData::SetReliable(bool state)
{
}

bool SVC_VoiceData::Process()
{
    return false;
}

bool SVC_VoiceData::ReadFromBuffer(bf_read& buffer)
{
    return false;
}

bool SVC_VoiceData::WriteToBuffer(bf_write& buffer)
{
    buffer.WriteUBitLong(svc_VoiceData, NETMSG_TYPE_BITS);
    buffer.WriteByte(m_nFromClient);
    buffer.WriteByte(m_bProximity);
    buffer.WriteWord(m_nLength);
    return buffer.WriteBits(m_DataOut, m_nLength);
}

bool SVC_VoiceData::IsReliable() const
{
    return false;
}

int SVC_VoiceData::GetType() const
{
    return 0;
}

int SVC_VoiceData::GetGroup() const
{
    return 0;
}

const char* SVC_VoiceData::GetName() const
{
    return nullptr;
}

INetChannel* SVC_VoiceData::GetNetChannel() const
{
    return nullptr;
}

const char* SVC_VoiceData::ToString() const
{
    return nullptr;
}

bool SVC_VoiceData::BIncomingMessageForProcessing(double, int) const
{
    return false;
}

int SVC_VoiceData::GetSize() const
{
    return 0;
}

void SVC_VoiceData::SetRatePolicy()
{
}
