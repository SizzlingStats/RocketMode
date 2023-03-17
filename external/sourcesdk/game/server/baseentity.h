
#pragma once

#include "../../public/iserverentity.h"

class ServerClass;
struct datamap_t;
struct ScriptClassDesc_t;
class CGameTrace;
typedef CGameTrace trace_t;
struct Ray_t;
class Vector;
class CCheckTransmitInfo;

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity : public IServerEntity
{
public:
    virtual ~CBaseEntity() = 0;

    // DECLARE_SERVERCLASS();
    virtual ServerClass* GetServerClass() = 0;
    virtual int YouForgotToImplementOrDeclareServerClass() = 0;

    // DECLARE_DATADESC();
    virtual datamap_t* GetDataDescMap() = 0;

#ifndef SDK_COMPAT
    virtual ScriptClassDesc_t* GetScriptDesc() = 0;
#endif

    virtual void SetModelIndexOverride(int index, int nValue) = 0;

    // virtual methods for derived classes to override
    virtual bool TestCollision(const Ray_t& ray, unsigned int mask, trace_t& trace) = 0;
    virtual	bool TestHitboxes(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr) = 0;
    virtual void ComputeWorldSpaceSurroundingBox(Vector* pWorldMins, Vector* pWorldMaxs) = 0;

    // Called by physics to see if we should avoid a collision test....
    virtual	bool ShouldCollide(int collisionGroup, int contentsMask) const = 0;

    virtual void SetOwnerEntity(CBaseEntity* pOwner) = 0;
#ifndef SDK_COMPAT
    virtual void SetScriptOwnerEntity(void* pOwner) = 0;
#endif

    // Only CBaseEntity implements these. CheckTransmit calls the virtual ShouldTransmit to see if the
    // entity wants to be sent. If so, it calls SetTransmit, which will mark any dependents for transmission too.
    virtual int ShouldTransmit(const CCheckTransmitInfo* pInfo) = 0;

    // Do NOT call this directly. Use DispatchUpdateTransmitState.
    virtual int UpdateTransmitState() = 0;

    // This marks the entity for transmission and passes the SetTransmit call to any dependents.
    virtual void SetTransmit(CCheckTransmitInfo* pInfo, bool bAlways) = 0;

    virtual const char* GetTracerType(void) = 0;

    // initialization
    virtual void Spawn() = 0;

    // Don't need the rest for now.
};
