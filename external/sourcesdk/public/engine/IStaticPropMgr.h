
#pragma once

class IPhysicsEnvironment;
class IVPhysicsKeyHandler;
struct Ray_t;
class CGameTrace;
typedef CGameTrace trace_t;
class IHandleEntity;
class CBaseHandle;
class ICollideable;
class Vector;
template<typename T> class CUtlMemory;
template<typename T, class A> class CUtlVector;

#define INTERFACEVERSION_STATICPROPMGR_SERVER "StaticPropMgrServer002"

class IStaticPropMgr
{
public:
    // Create physics representations of props
    virtual void CreateVPhysicsRepresentations( IPhysicsEnvironment *physenv, IVPhysicsKeyHandler *pDefaults, void *pGameData ) = 0;

    // Purpose: Trace a ray against the specified static Prop. Returns point of intersection in trace_t
    virtual void TraceRayAgainstStaticProp( const Ray_t& ray, int staticPropIndex, trace_t& tr ) = 0;

    // Is a base handle a static prop?
    virtual bool IsStaticProp( IHandleEntity *pHandleEntity ) const = 0;
    virtual bool IsStaticProp( CBaseHandle handle ) const = 0;

    // returns a collideable interface to static props
    virtual ICollideable *GetStaticPropByIndex( int propIndex ) = 0;
};

class IStaticPropMgrServer : public IStaticPropMgr
{
public:
    //Changes made specifically to support the Portal mod (smack Dave Kircher if something breaks) (Added separately to both client and server to not mess with versioning)
    //===================================================================
    virtual void GetAllStaticProps( CUtlVector<ICollideable *> *pOutput ) = 0; //testing function that will eventually be removed
    virtual void GetAllStaticPropsInAABB( const Vector &vMins, const Vector &vMaxs, CUtlVector<ICollideable *> *pOutput ) = 0; //get all static props that exist wholly or partially in an AABB
    virtual void GetAllStaticPropsInOBB( const Vector &ptOrigin, const Vector &vExtent1, const Vector &vExtent2, const Vector &vExtent3, CUtlVector<ICollideable *> *pOutput ) = 0; //get all static props that exist wholly or partially in an OBB
    //===================================================================
};
