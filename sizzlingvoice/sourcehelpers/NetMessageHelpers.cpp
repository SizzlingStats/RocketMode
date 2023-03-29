
#include "NetMessageHelpers.h"
#include "sourcesdk/common/protocol.h"
#include "sourcesdk/engine/net.h"
#include "sourcesdk/public/tier1/bitbuf.h"

DEFINE_INHERITED_DESTRUCTOR(SetSingleConvar, INetMessage);
void SetSingleConvar::SetNetChannel(INetChannel* netchan) { }
void SetSingleConvar::SetReliable(bool state) { }
bool SetSingleConvar::Process(void) { return false; }
bool SetSingleConvar::ReadFromBuffer(bf_read& buffer) { return false; }

bool SetSingleConvar::WriteToBuffer(bf_write& buffer)
{
    buffer.WriteUBitLong(net_SetConVar, NETMSG_TYPE_BITS);
    buffer.WriteByte(1); // num convars
    buffer.WriteString(mName);
    buffer.WriteString(mValue);
    return !buffer.IsOverflowed();
}

bool SetSingleConvar::IsReliable(void) const { return false; }
int SetSingleConvar::GetType(void) const { return 0; }
int SetSingleConvar::GetGroup(void) const { return 0; }
const char* SetSingleConvar::GetName(void) const { return nullptr; }
INetChannel* SetSingleConvar::GetNetChannel(void) const { return nullptr; }
const char* SetSingleConvar::ToString(void) const { return nullptr; }
bool SetSingleConvar::BIncomingMessageForProcessing(double, int) const { return false; }
int SetSingleConvar::GetSize() const { return 0; }
void SetSingleConvar::SetRatePolicy() { }
