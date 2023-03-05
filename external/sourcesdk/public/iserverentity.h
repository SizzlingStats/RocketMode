
#pragma once

#include "iserverunknown.h"
#include "string_t.h"

// This class is how the engine talks to entities in the game DLL.
// CBaseEntity implements this interface.
class IServerEntity : public IServerUnknown
{
public:
    virtual ~IServerEntity() = 0;

    // Previously in pev
    virtual int GetModelIndex() const = 0;
    virtual string_t GetModelName() const = 0;

    virtual void SetModelIndex(int index) = 0;
};
