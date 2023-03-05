
#pragma once

class IHandleEntity;
class ServerClass;
struct edict_t;
class CBaseNetworkable;
class CBaseEntity;
struct PVSInfo_t;

class IServerNetworkable
{
    // These functions are handled automatically by the server_class macros and CBaseNetworkable.
public:
    // Gets at the entity handle associated with the collideable
    virtual IHandleEntity* GetEntityHandle() = 0;

    // Tell the engine which class this object is.
    virtual ServerClass* GetServerClass() = 0;

    virtual edict_t* GetEdict() const = 0;

    virtual const char* GetClassName() const = 0;
    virtual void Release() = 0;

    virtual int AreaNum() const = 0;

    // In place of a generic QueryInterface.
    virtual CBaseNetworkable* GetBaseNetworkable() = 0;
    virtual CBaseEntity* GetBaseEntity() = 0; // Only used by game code.
    virtual PVSInfo_t* GetPVSInfo() = 0; // get current visibilty data

protected:
    // Should never call delete on this! 
    virtual ~IServerNetworkable() = 0;
};
