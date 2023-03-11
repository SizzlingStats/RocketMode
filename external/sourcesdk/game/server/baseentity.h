
#pragma once

#include "../../public/iserverentity.h"

class ServerClass;
struct datamap_t;
class CGameTrace;
typedef CGameTrace trace_t;
struct Ray_t;

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

    virtual void SetModelIndexOverride(int index, int nValue) = 0;

    // virtual methods for derived classes to override
    virtual bool TestCollision(const Ray_t& ray, unsigned int mask, trace_t& trace) = 0;
    virtual	bool TestHitboxes(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr) = 0;
    virtual void ComputeWorldSpaceSurroundingBox(Vector* pWorldMins, Vector* pWorldMaxs) = 0;

    // Called by physics to see if we should avoid a collision test....
    virtual	bool ShouldCollide(int collisionGroup, int contentsMask) const = 0;

    virtual void SetOwnerEntity(CBaseEntity* pOwner) = 0;

    // Don't need the rest for now.
};
