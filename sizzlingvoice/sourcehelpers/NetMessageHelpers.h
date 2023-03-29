
#pragma once

#include "sourcesdk/public/inetmessage.h"
#include "sourcesdk/public/tier0/platform.h"

// Uses type NET_SetConVar
class SetSingleConvar : public INetMessage
{
public:
    DECL_INHERITED_DESTRUCTOR(INetMessage);

    virtual void SetNetChannel(INetChannel* netchan);
    virtual void SetReliable(bool state);

    virtual bool Process(void);

    virtual	bool ReadFromBuffer(bf_read& buffer);
    virtual	bool WriteToBuffer(bf_write& buffer);

    virtual bool IsReliable(void) const;

    virtual int GetType(void) const;
    virtual int GetGroup(void) const;
    virtual const char* GetName(void) const;
    virtual INetChannel* GetNetChannel(void) const;
    virtual const char* ToString(void) const;

    virtual bool BIncomingMessageForProcessing(double, int) const;
    virtual int GetSize() const;
    virtual void SetRatePolicy();

    void Set(const char* name, const char* value)
    {
        mName = name;
        mValue = value;
    }

private:
    const char* mName;
    const char* mValue;
};
