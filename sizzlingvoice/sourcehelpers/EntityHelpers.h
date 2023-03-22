
#pragma once

#include "sourcesdk/public/string_t.h"
#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/public/mathlib/vector.h"
#include <assert.h>

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
class CEconItemView;
class CAttributeContainer;

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
    extern int sTeamNumOffset;          // m_iTeamNum

    void InitializeOffsets(CBaseEntity* ent);

    // m_iClassname
    inline string_t GetClassname(CBaseEntity* ent)
    {
        assert(sClassnameOffset > 0);
        return *(string_t*)((char*)ent + sClassnameOffset);
    }

    // m_iName
    inline string_t GetName(CBaseEntity* ent)
    {
        assert(sNameOffset > 0);
        return *(string_t*)((char*)ent + sNameOffset);
    }

    // m_ModelName
    inline void SetModelName(CBaseEntity* ent, string_t modelName)
    {
        assert(sModelNameOffset > 0);
        *(string_t*)((char*)ent + sModelNameOffset) = modelName;
    }

    // m_hOwnerEntity
    inline CBaseHandle& GetOwnerEntity(CBaseEntity* ent)
    {
        assert(sOwnerEntityOffset > 0);
        return *(CBaseHandle*)((char*)ent + sOwnerEntityOffset);
    }

    // m_fFlags. TODO: what happens if derived classes didn't ask for m_fFlags replication?
    inline void AddFlag(CBaseEntity* ent, int flags, IServerGameEnts* gameEnts, IVEngineServer* engineServer)
    {
        assert(sFFlagsOffset > 0);
        const int offset = sFFlagsOffset;
        *(int*)((char*)ent + offset) |= flags;

        EntityHelpers::StateChanged(ent, offset, gameEnts, engineServer);
    }

    // m_fFlags. TODO: what happens if derived classes didn't ask for m_fFlags replication?
    inline void RemoveFlag(CBaseEntity* ent, int flags, IServerGameEnts* gameEnts, IVEngineServer* engineServer)
    {
        assert(sFFlagsOffset > 0);
        const int offset = sFFlagsOffset;
        *(int*)((char*)ent + offset) &= ~flags;

        EntityHelpers::StateChanged(ent, offset, gameEnts, engineServer);
    }

    // m_iEFlags
    inline void AddEFlags(CBaseEntity* ent, int flags)
    {
        assert(sEFlagsOffset > 0);
        int& eflags = *(int*)((char*)ent + sEFlagsOffset);
        eflags |= flags;
    }

    // m_vecVelocity
    inline void SetLocalVelocity(CBaseEntity* ent, const Vector& localVelocity)
    {
        assert(sLocalVelocityOffset > 0);
        Vector& velocity = *(Vector*)((char*)ent + sLocalVelocityOffset);
        velocity = localVelocity;

        // TODO: invalidate children too
        AddEFlags(ent, EFL_DIRTY_ABSVELOCITY);
    }

    // m_vecVelocity
    inline const Vector& GetLocalVelocity(CBaseEntity* ent)
    {
        assert(sLocalVelocityOffset > 0);
        return *(Vector*)((char*)ent + sLocalVelocityOffset);
    }

    // m_angRotation
    inline void SetLocalRotation(CBaseEntity* ent, const QAngle& localRotation)
    {
        assert(sAngRotationOffset > 0);
        // TODO: EFL_DIRTY_ABSTRANSFORM.
        // InvalidatePhysicsRecursive(ANGLES_CHANGED)
        *(QAngle*)((char*)ent + sAngRotationOffset) = localRotation;
    }

    // m_angRotation
    inline const QAngle& GetLocalRotation(CBaseEntity* ent)
    {
        assert(sAngRotationOffset > 0);
        return *(QAngle*)((char*)ent + sAngRotationOffset);
    }

    // m_vecAngVelocity
    inline void SetLocalAngularVelocity(CBaseEntity* ent, const QAngle& localAngVel)
    {
        assert(sAngVelocityOffset > 0);
        QAngle& angVel = *(QAngle*)((char*)ent + sAngVelocityOffset);
        angVel = localAngVel;
        // no invalidation needed here.
    }

    // m_vecAngVelocity
    inline const QAngle& GetLocalAngularVelocity(CBaseEntity* ent)
    {
        assert(sAngVelocityOffset > 0);
        return *(QAngle*)((char*)ent + sAngVelocityOffset);
    }

    // m_target
    inline string_t GetTarget(CBaseEntity* ent)
    {
        assert(sTargetOffset > 0);
        return *(string_t*)((char*)ent + sTargetOffset);
    }

    // m_pAttributes
    inline IHasAttributes* GetAttribInterface(CBaseEntity* ent)
    {
        assert(sAttributesOffset > 0);
        return *(IHasAttributes**)((char*)ent + sAttributesOffset);
    }

    // m_iTeamNum
    inline int GetTeam(CBaseEntity* ent)
    {
        assert(sTeamNumOffset > 0);
        return *(int*)((char*)ent + sTeamNumOffset);
    }
}

namespace BasePlayerHelpers
{
    extern int sObserverModeOffset;     // m_iObserverMode
    extern int sObserverTargetOffset;   // m_hObserverTarget

    void InitializeOffsets(CBaseEntity* ent);

    // m_iObserverMode
    inline int GetObserverMode(CBaseEntity* ent)
    {
        assert(sObserverModeOffset > 0);
        return *(int*)((char*)ent + sObserverModeOffset);
    }

    // m_hObserverTarget
    inline const CBaseHandle& GetObserverTarget(CBaseEntity* ent)
    {
        assert(sObserverTargetOffset > 0);
        return *(CBaseHandle*)((char*)ent + sObserverTargetOffset);
    }
}

namespace TFBaseRocketHelpers
{
    extern int sLauncherOffset;     // m_hLauncher

    void InitializeOffsets(CBaseEntity* ent);

    // m_hLauncher
    inline const CBaseHandle& GetLauncher(CBaseEntity* ent)
    {
        assert(sLauncherOffset > 0);
        return *(CBaseHandle*)((char*)ent + sLauncherOffset);
    }
}

namespace TFDroppedWeaponHelpers
{
    extern int sItemOffset;     // m_Item

    void InitializeOffsets(CBaseEntity* ent);

    // m_Item
    inline CEconItemView& GetItem(CBaseEntity* ent)
    {
        assert(sItemOffset > 0);
        return *(CEconItemView*)((char*)ent + sItemOffset);
    }
}

namespace AttributeContainerHelpers
{
    extern int sItemOffset;     // m_Item

    void InitializeOffsets(CAttributeContainer* container);

    // m_Item
    inline CEconItemView& GetItem(CAttributeContainer* container)
    {
        assert(sItemOffset > 0);
        return *(CEconItemView*)((char*)container + sItemOffset);
    }
}
