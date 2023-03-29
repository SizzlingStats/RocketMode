
#pragma once

#include "sourcesdk/public/inetmessage.h"
#include "sourcesdk/public/tier0/platform.h"

// Uses type NET_SetConVar
class SetSingleConvar : public INetMessage
{
public:
    DECL_INHERITED_DESTRUCTOR(INetMessage);

    virtual void SetNetChannel(INetChannel* netchan) override;
    virtual void SetReliable(bool state) override;

    virtual bool Process() override;

    virtual	bool ReadFromBuffer(bf_read& buffer) override;
    virtual	bool WriteToBuffer(bf_write& buffer) override;

    virtual bool IsReliable() const override;

    virtual int GetType() const override;
    virtual int GetGroup() const override;
    virtual const char* GetName() const override;
    virtual INetChannel* GetNetChannel() const override;
    virtual const char* ToString() const override;

    virtual bool BIncomingMessageForProcessing(double, int) const override;
    virtual int GetSize() const override;
    virtual void SetRatePolicy() override;

    void Set(const char* name, const char* value)
    {
        mName = name;
        mValue = value;
    }

private:
    const char* mName;
    const char* mValue;
};
