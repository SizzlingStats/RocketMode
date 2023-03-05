
#pragma once

class CBaseHandle;

// An IHandleEntity-derived class can go into an entity list and use ehandles.
class IHandleEntity
{
public:
    virtual ~IHandleEntity() = 0;
    virtual void SetRefEHandle(const CBaseHandle& handle) = 0;
    virtual const CBaseHandle& GetRefEHandle() const = 0;
};
