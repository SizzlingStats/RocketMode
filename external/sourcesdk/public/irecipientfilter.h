
#pragma once

class IRecipientFilter
{
public:
    virtual void IRecipientFilter_Destructor() = 0;

    virtual bool IsReliable() const = 0;
    virtual bool IsInitMessage() const = 0;

    virtual int GetRecipientCount() const = 0;
    virtual int GetRecipientIndex(int slot) const = 0;
};
