
#pragma once

#include "sourcesdk/public/string_t.h"
#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/public/mathlib/vector.h"

struct datamap_t;
class CBaseEntity;
struct edict_t;
class IVEngineServer;
class IServerGameDLL;
class ServerClass;
class SendTable;
class SendProp;
class CBaseHandle;
class IServerGameEnts;
class IServerTools;
class IHasAttributes;

namespace EntityHelpers
{
    // Datamaps
    int GetDatamapVarOffset(datamap_t* pDatamap, const char* szVarName);
    void PrintDatamap(datamap_t* datamap);

    // Netprops
    // Call this after manually editing a net prop
    void StateChanged(edict_t* edict, unsigned short offset, IVEngineServer* engineServer);
    void StateChanged(CBaseEntity* ent, unsigned short offset, IServerGameEnts* gameEnts, IVEngineServer* engineServer);
    void FullStateChanged(edict_t* edict, IVEngineServer* engineServer);

    // SendTables
    void PrintAllServerClassTables(IServerGameDLL* serverGameDll);
    void PrintServerClassTables(IServerGameDLL* serverGameDll, const char* classname);

    ServerClass* GetServerClass(IServerGameDLL* serverGameDll, const char* classname);

    SendTable* GetTable(IServerGameDLL* serverGameDll, const char* className, const char* tableName);
    SendTable* GetTable(ServerClass* serverClass, const char* tableName);

    SendProp* GetProp(IServerGameDLL* serverGameDll, const char* className, const char* tableName, const char* propName);
    SendProp* GetProp(ServerClass* serverClass, const char* tableName, const char* propName);
    SendProp* GetProp(SendTable* table, const char* propName);

    CBaseEntity* HandleToEnt(const CBaseHandle& handle, IServerTools* serverTools);
}

namespace BaseEntityHelpers
{
    extern int sClassnameOffset;        // m_iClassname
    extern int sNameOffset;             // m_iName
    extern int sModelNameOffset;        // m_ModelName
    extern int sOwnerEntityOffset;      // m_hOwnerEntity
    extern int sFFlagsOffset;           // m_fFlags
    extern int sEFlagsOffset;           // m_iEFlags
    extern int sLocalVelocityOffset;    // m_vecVelocity
    extern int sAngRotationOffset;      // m_angRotation
    extern int sAngVelocityOffset;      // m_vecAngVelocity
    extern int sTargetOffset;           // m_target
    extern int sAttributesOffset;       // m_pAttributes

    void InitializeOffsets(CBaseEntity* ent);

    // m_iClassname
    inline string_t GetClassname(CBaseEntity* ent)
    {
        return *(string_t*)((char*)ent + sClassnameOffset);
    }

    // m_iName
    inline string_t GetName(CBaseEntity* ent)
    {
        return *(string_t*)((char*)ent + sNameOffset);
    }

    // m_ModelName
    inline void SetModelName(CBaseEntity* ent, string_t modelName)
    {
        *(string_t*)((char*)ent + sModelNameOffset) = modelName;
    }

    // m_hOwnerEntity
    inline CBaseHandle& GetOwnerEntity(CBaseEntity* ent)
    {
        return *(CBaseHandle*)((char*)ent + sOwnerEntityOffset);
    }

    // m_fFlags. TODO: what happens if derived classes didn't ask for m_fFlags replication?
    inline void AddFlag(CBaseEntity* ent, int flags, IServerGameEnts* gameEnts, IVEngineServer* engineServer)
    {
        const int offset = sFFlagsOffset;
        *(int*)((char*)ent + offset) |= flags;

        EntityHelpers::StateChanged(ent, offset, gameEnts, engineServer);
    }

    // m_fFlags. TODO: what happens if derived classes didn't ask for m_fFlags replication?
    inline void RemoveFlag(CBaseEntity* ent, int flags, IServerGameEnts* gameEnts, IVEngineServer* engineServer)
    {
        const int offset = sFFlagsOffset;
        *(int*)((char*)ent + offset) &= ~flags;

        EntityHelpers::StateChanged(ent, offset, gameEnts, engineServer);
    }

    // m_iEFlags
    inline void AddEFlags(CBaseEntity* ent, int flags)
    {
        int& eflags = *(int*)((char*)ent + sEFlagsOffset);
        eflags |= flags;
    }

    // m_vecVelocity
    inline void SetLocalVelocity(CBaseEntity* ent, const Vector& localVelocity)
    {
        Vector& velocity = *(Vector*)((char*)ent + sLocalVelocityOffset);
        velocity = localVelocity;

        // TODO: invalidate children too
        AddEFlags(ent, EFL_DIRTY_ABSVELOCITY);
    }

    // m_vecVelocity
    inline const Vector& GetLocalVelocity(CBaseEntity* ent)
    {
        return *(Vector*)((char*)ent + sLocalVelocityOffset);
    }

    // m_angRotation
    inline void SetLocalRotation(CBaseEntity* ent, const QAngle& localRotation)
    {
        // TODO: EFL_DIRTY_ABSTRANSFORM.
        // InvalidatePhysicsRecursive(ANGLES_CHANGED)
        *(QAngle*)((char*)ent + sAngRotationOffset) = localRotation;
    }

    // m_angRotation
    inline const QAngle& GetLocalRotation(CBaseEntity* ent)
    {
        return *(QAngle*)((char*)ent + sAngRotationOffset);
    }

    // m_vecAngVelocity
    inline void SetLocalAngularVelocity(CBaseEntity* ent, const QAngle& localAngVel)
    {
        QAngle& angVel = *(QAngle*)((char*)ent + sAngVelocityOffset);
        angVel = localAngVel;
        // no invalidation needed here.
    }

    // m_vecAngVelocity
    inline const QAngle& GetLocalAngularVelocity(CBaseEntity* ent)
    {
        return *(QAngle*)((char*)ent + sAngVelocityOffset);
    }

    // m_target
    inline string_t GetTarget(CBaseEntity* ent)
    {
        return *(string_t*)((char*)ent + sTargetOffset);
    }

    // m_pAttributes
    inline IHasAttributes* GetAttribInterface(CBaseEntity* ent)
    {
        return (IHasAttributes*)((char*)ent + sAttributesOffset);
    }
}
