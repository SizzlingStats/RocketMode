
#include "EntityHelpers.h"
#include "Debug.h"

#include "sourcesdk/game/server/baseentity.h"
#include "sourcesdk/public/basehandle.h"
#include "sourcesdk/public/datamap.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/server_class.h"
#include "sourcesdk/public/toolframework/itoolentity.h"
#include "sourcesdk/game/shared/econ/attribute_manager.h"
#include "sourcesdk/game/shared/gamerules.h"
#include "base/stringbuilder.h"
#include "hde32/hde32.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

template<typename T, int N>
constexpr int ArrayLength(const T(&)[N]) { return N; }

int EntityHelpers::GetDatamapVarOffset(datamap_t* pDatamap, const char* szVarName)
{
    while (pDatamap)
    {
        const int numFields = pDatamap->dataNumFields;
        typedescription_t* pFields = pDatamap->dataDesc;
        for (int i = 0; i < numFields; ++i)
        {
            typedescription_t* pField = &pFields[i];
            if (pField->fieldName && !strcmp(pField->fieldName, szVarName))
            {
                return pField->fieldOffset[TD_OFFSET_NORMAL];
            }
            else if (pField->td)
            {
                // there can be additional data tables inside this type description
                const int offset = GetDatamapVarOffset(pField->td, szVarName);
                if (offset > 0)
                {
                    return offset;
                }
            }
        }
        pDatamap = pDatamap->baseMap;
    }
    return 0;
}

static void PrintDatamapRecursive(datamap_t* datamap, StringBuilder<128>& spacing)
{
    Debug::Msg("%s%s\n", spacing.c_str(), datamap->dataClassName);
    spacing.Append("  |");

    if (datamap->baseMap)
    {
        PrintDatamapRecursive(datamap->baseMap, spacing);
    }

    const int numFields = datamap->dataNumFields;
    typedescription_t* pFields = datamap->dataDesc;
    for (int i = 0; i < numFields; ++i)
    {
        typedescription_t* pField = &pFields[i];
        if (pField->fieldType != FIELD_VOID)
        {
            const char* name = pField->fieldName;
            const int offset = pField->fieldOffset[TD_OFFSET_NORMAL];
            Debug::Msg("%s%s, Offset: %i (%i bytes)\n", spacing.c_str(), name, offset, pField->fieldSizeInBytes);

            if (pField->td)
            {
                spacing.Append("  |");
                PrintDatamapRecursive(pField->td, spacing);
                spacing.Reduce(3);
            }
        }
    }
    spacing.Reduce(3);
}

void EntityHelpers::PrintDatamap(datamap_t* datamap)
{
    StringBuilder<128> spacing;
    PrintDatamapRecursive(datamap, spacing);
    Debug::Msg("\n");
}

void EntityHelpers::FullStateChanged(edict_t* edict, IVEngineServer* engineServer)
{
    edict->m_fStateFlags |= (FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
    IChangeInfoAccessor* accessor = engineServer->GetChangeAccessor(edict);
    accessor->SetChangeInfoSerialNumber(0);
}

void EntityHelpers::StateChanged(edict_t* edict, unsigned short offset, IVEngineServer* engineServer)
{
    if (edict->m_fStateFlags & FL_FULL_EDICT_CHANGED)
        return;

    edict->m_fStateFlags |= FL_EDICT_CHANGED;

    IChangeInfoAccessor* accessor = engineServer->GetChangeAccessor(edict);
    CSharedEdictChangeInfo* sharedChangeInfo = engineServer->GetSharedEdictChangeInfo();

    if (accessor->GetChangeInfoSerialNumber() == sharedChangeInfo->m_iSerialNumber)
    {
        // Ok, I still own this one.
        CEdictChangeInfo* p = &sharedChangeInfo->m_ChangeInfos[accessor->GetChangeInfo()];

        // Now add this offset to our list of changed variables.		
        for (unsigned short i = 0; i < p->m_nChangeOffsets; i++)
            if (p->m_ChangeOffsets[i] == offset)
                return;

        if (p->m_nChangeOffsets == MAX_CHANGE_OFFSETS)
        {
            // Invalidate our change info.
            accessor->SetChangeInfoSerialNumber(0);
            edict->m_fStateFlags |= FL_FULL_EDICT_CHANGED; // So we don't get in here again.
        }
        else
        {
            p->m_ChangeOffsets[p->m_nChangeOffsets++] = offset;
        }
    }
    else
    {
        if (sharedChangeInfo->m_nChangeInfos == MAX_EDICT_CHANGE_INFOS)
        {
            // Shucks.. have to mark the edict as fully changed because we don't have room to remember this change.
            accessor->SetChangeInfoSerialNumber(0);
            edict->m_fStateFlags |= FL_FULL_EDICT_CHANGED;
        }
        else
        {
            // Get a new CEdictChangeInfo and fill it out.
            accessor->SetChangeInfo(sharedChangeInfo->m_nChangeInfos);
            sharedChangeInfo->m_nChangeInfos++;

            accessor->SetChangeInfoSerialNumber(sharedChangeInfo->m_iSerialNumber);

            CEdictChangeInfo* p = &sharedChangeInfo->m_ChangeInfos[accessor->GetChangeInfo()];
            p->m_ChangeOffsets[0] = offset;
            p->m_nChangeOffsets = 1;
        }
    }
}

void EntityHelpers::StateChanged(CBaseEntity* ent, unsigned short offset, IServerGameEnts* gameEnts, IVEngineServer* engineServer)
{
    edict_t* edict = gameEnts->BaseEntityToEdict(ent);
    if (edict)
    {
        StateChanged(edict, offset, engineServer);
    }
}

static const char* sPropTypeNames[7] =
{
    "int", "float", "vector", "vectorxy", "string", "array", "datatable"
};

static void RecurseServerTable(SendTable* pTable, StringBuilder<128>& spacing)
{
    Debug::Msg("%s%s\n", spacing.c_str(), pTable->GetName());
    spacing.Append("  |");

    int num = pTable->GetNumProps();
    for (int i = 0; i < num; i++)
    {
        SendProp* pProp = pTable->GetProp(i);
        SendPropType PropType = pProp->m_Type;
        const char* typeString = nullptr;
        if (PropType < ArrayLength(sPropTypeNames))
        {
            typeString = sPropTypeNames[PropType];
        }

        Debug::Msg("%s%s, Offset: %i (%s, %i bits)\n",
            spacing.c_str(), pProp->GetName(), pProp->GetOffset(), typeString, pProp->m_nBits);

        SendTable* subTable = pProp->GetDataTable();
        if (subTable)
        {
            spacing.Append("  |");
            RecurseServerTable(subTable, spacing);
        }
    }
    spacing.Reduce(6);
}

static void PrintServerClass(ServerClass* serverclass)
{
    Debug::Msg("%s\n", serverclass->GetName());
    SendTable* table = serverclass->GetTable();
    if (table)
    {
        StringBuilder<128> spacing;
        spacing.Append("  |");
        RecurseServerTable(table, spacing);
        Debug::Msg("\n");
    }
}

void EntityHelpers::PrintAllServerClassTables(IServerGameDLL* serverGameDll)
{
    ServerClass* pClass = serverGameDll->GetAllServerClasses();
    while (pClass)
    {
        PrintServerClass(pClass);
        pClass = pClass->GetNext();
    }
}

void EntityHelpers::PrintServerClassTables(IServerGameDLL* serverGameDll, const char* classname)
{
    ServerClass* pClass = GetServerClass(serverGameDll, classname);
    if (pClass)
    {
        PrintServerClass(pClass);
    }
}

ServerClass* EntityHelpers::GetServerClass(IServerGameDLL* serverGameDll, const char* classname)
{
    ServerClass* pClass = serverGameDll->GetAllServerClasses();
    while (pClass)
    {
        const char* name = pClass->GetName();
        if (!strcmp(classname, name))
        {
            return pClass;
        }
        pClass = pClass->m_pNext;
    }
    return nullptr;
}

static SendTable* GetTableRecursive(SendTable* table, const char* name)
{
    if (!table)
    {
        return nullptr;
    }

    if (!strcmp(name, table->GetName()))
    {
        return table;
    }

    const int numProps = table->GetNumProps();
    for (int i = 0; i < numProps; ++i)
    {
        SendProp* prop = table->GetProp(i);
        SendTable* foundSubTable = GetTableRecursive(prop->GetDataTable(), name);
        if (foundSubTable)
        {
            return foundSubTable;
        }
    }
    return nullptr;
}

SendTable* EntityHelpers::GetTable(IServerGameDLL* serverGameDll, const char* className, const char* tableName)
{
    ServerClass* pClass = GetServerClass(serverGameDll, className);
    if (pClass)
    {
        SendTable* table = GetTableRecursive(pClass->GetTable(), tableName);
        return table;
    }
    return nullptr;
}

SendTable* EntityHelpers::GetTable(ServerClass* serverClass, const char* tableName)
{
    return GetTableRecursive(serverClass->GetTable(), tableName);
}

SendProp* EntityHelpers::GetProp(IServerGameDLL* serverGameDll, const char* className, const char* tableName, const char* propName)
{
    ServerClass* pClass = GetServerClass(serverGameDll, className);
    if (pClass)
    {
        SendProp* prop = GetProp(pClass, tableName, propName);
        return prop;
    }
    return nullptr;
}

SendProp* EntityHelpers::GetProp(ServerClass* serverClass, const char* tableName, const char* propName)
{
    SendTable* table = GetTableRecursive(serverClass->GetTable(), tableName);
    if (table)
    {
        SendProp* prop = EntityHelpers::GetProp(table, propName);
        return prop;
    }
    return nullptr;
}

SendProp* EntityHelpers::GetProp(SendTable* table, const char* propName)
{
    const int numProps = table->GetNumProps();
    for (int i = 0; i < numProps; ++i)
    {
        SendProp* prop = table->GetProp(i);
        if ((prop->m_Flags & SPROP_EXCLUDE) != 0)
        {
            continue;
        }
        if (!strcmp(prop->GetName(), propName))
        {
            return prop;
        }
    }
    return nullptr;
}

CBaseEntity* EntityHelpers::HandleToEnt(const CBaseHandle& handle, IServerTools* serverTools)
{
    CBaseEntity* ent = serverTools->GetBaseEntityByEntIndex(handle.GetEntryIndex());
    if (ent && (ent->GetRefEHandle() == handle))
    {
        return ent;
    }
    return nullptr;
}

int BaseEntityHelpers::sClassnameOffset;
int BaseEntityHelpers::sNameOffset;
int BaseEntityHelpers::sModelNameOffset;
int BaseEntityHelpers::sOwnerEntityOffset;
int BaseEntityHelpers::sFFlagsOffset;
int BaseEntityHelpers::sEFlagsOffset;
int BaseEntityHelpers::sLocalVelocityOffset;
int BaseEntityHelpers::sAngRotationOffset;
int BaseEntityHelpers::sAbsOriginOffset;
int BaseEntityHelpers::sAbsVelocityOffset;
int BaseEntityHelpers::sAngVelocityOffset;
int BaseEntityHelpers::sTargetOffset;
int BaseEntityHelpers::sAttributesOffset;
int BaseEntityHelpers::sTeamNumOffset;

void BaseEntityHelpers::InitializeOffsets(CBaseEntity* ent)
{
    if (sClassnameOffset > 0)
    {
        // already initialized
        return;
    }

    datamap_t* datamap = ent->GetDataDescMap();
    assert(datamap);

    //EntityHelpers::PrintDatamap(datamap);

    sClassnameOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_iClassname");
    assert(sClassnameOffset > 0);

    sNameOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_iName");
    assert(sNameOffset > 0);

    sModelNameOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_ModelName");
    assert(sModelNameOffset > 0);

    sOwnerEntityOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_hOwnerEntity");
    assert(sOwnerEntityOffset > 0);

    sFFlagsOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_fFlags");
    assert(sFFlagsOffset > 0);

    sEFlagsOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_iEFlags");
    assert(sEFlagsOffset > 0);

    sLocalVelocityOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_vecVelocity");
    assert(sLocalVelocityOffset > 0);

    sAngRotationOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_angRotation");
    assert(sAngRotationOffset > 0);

    sAbsOriginOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_vecAbsOrigin");
    assert(sAbsOriginOffset > 0);

    sAbsVelocityOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_vecAbsVelocity");
    assert(sAbsVelocityOffset > 0);

    sAngVelocityOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_vecAngVelocity");
    assert(sAngVelocityOffset > 0);

    sTargetOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_target");
    assert(sTargetOffset > 0);

    // excerpt from CBaseEntity
    struct GetAttribInterface_Hack
    {
        // NOTE: m_pAttributes needs to be set in the leaf class constructor.
        IHasAttributes* m_pAttributes;
        CBaseEntity* m_pLink;// used for temporary link-list operations. 
        // variables promoted from edict_t
        string_t m_target;
    };
    sAttributesOffset = sTargetOffset - offsetof(GetAttribInterface_Hack, m_target);
    assert(sAttributesOffset > 0);

    sTeamNumOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_iTeamNum");
    assert(sTeamNumOffset > 0);
}

int BasePlayerHelpers::sObserverModeOffset;
int BasePlayerHelpers::sObserverLastModeOffset;
int BasePlayerHelpers::sObserverTargetOffset;
int BasePlayerHelpers::sForcedObserverModeOffset;
int BasePlayerHelpers::sPlayerNameOffset;

void BasePlayerHelpers::InitializeOffsets(CBaseEntity* player)
{
    if (sObserverModeOffset > 0)
    {
        // already initialized
        return;
    }

    datamap_t* datamap = player->GetDataDescMap();
    assert(datamap);

    sObserverModeOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_iObserverMode");
    assert(sObserverModeOffset > 0);

    sObserverLastModeOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_iObserverLastMode");
    assert(sObserverLastModeOffset > 0);

    sObserverTargetOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_hObserverTarget");
    assert(sObserverTargetOffset > 0);

    sForcedObserverModeOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_bForcedObserverMode");
    assert(sForcedObserverModeOffset > 0);

    sPlayerNameOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_szNetname");
    assert(sPlayerNameOffset > 0);
}

void BasePlayerHelpers::SetObserverTarget(CBaseEntity* player, const CBaseHandle& target, IServerGameEnts* gameEnts, IVEngineServer* engineServer)
{
    assert(sObserverTargetOffset > 0);
    const int offset = sObserverTargetOffset;
    CBaseHandle& observerTarget = *(CBaseHandle*)((char*)player + sObserverTargetOffset);
    if (observerTarget != target)
    {
        observerTarget = target;
        EntityHelpers::StateChanged(player, offset, gameEnts, engineServer);
    }
}

int TFPlayerHelpers::sPlayerObjectsOffset;
int TFPlayerHelpers::sObservableEntitiesOffset;

void TFPlayerHelpers::InitializeOffsets(IServerGameDLL* serverGameDll)
{
    if (sPlayerObjectsOffset > 0)
    {
        // already initialized
        return;
    }

    ServerClass* tfPlayerClass = EntityHelpers::GetServerClass(serverGameDll, "CTFPlayer");
    assert(tfPlayerClass);

    SendProp* playerObjectArray = EntityHelpers::GetProp(tfPlayerClass, "DT_TFLocalPlayerExclusive", "\"player_object_array\"");
    assert(playerObjectArray);

    SendTable* tfPlayerTable = EntityHelpers::GetTable(tfPlayerClass, "DT_TFPlayer");
    assert(tfPlayerTable);

#ifdef SDK_COMPAT
    constexpr const char* lastDamageName = "m_flLastDamageTime";
#else
    constexpr const char* lastDamageName = "m_flMvMLastDamageTime";
#endif
    SendProp* flMVMLastDamageTimeProp = EntityHelpers::GetProp(tfPlayerTable, lastDamageName);
    assert(flMVMLastDamageTimeProp);
    const int flMVMLastDamageTimeOffset = flMVMLastDamageTimeProp->GetOffset();

    SendProp* bInPowerPlayProp = EntityHelpers::GetProp(tfPlayerTable, "m_bInPowerPlay");
    assert(bInPowerPlayProp);
    const int bInPowerPlayOffset = bInPowerPlayProp->GetOffset();

    // verification for m_aObjects offset:
    // class CTFPlayer {
    // net prop m_flMvMLastDamageTime
    // ...
    // m_aObjects
    // ...
    // net prop m_bInPowerPlay
    // 
    // filter for offsets between those two net props.

    ArrayLengthSendProxyFn lengthProxyFn = playerObjectArray->GetArrayLengthProxy();

    int playerObjectsCountOffset = 0;
    // scan 10 instructions at SendProxyArrayLength_PlayerObjects
    // looking for the disp32 offset.
    //
    // arg_0 = dword ptr 8
    // 55                   push    ebp
    // 8B EC                mov     ebp, esp
    // 8B 45 08             mov     eax, [ebp + arg_0]
    // 8B 80 BC 22 00 00    mov     eax, [eax + 22BCh] <- looking for this
    // 5D                   pop     ebp
    // C3                   retn
    //
    unsigned char* instr = reinterpret_cast<unsigned char*>(lengthProxyFn);
    hde32s hs;
    for (int i = 0; i < 10; ++i)
    {
        hde32_disasm(instr, &hs);
        if ((hs.flags & F_ERROR) != 0)
        {
            break;
        }
        if ((hs.flags & F_DISP32) != 0)
        {
            const int offset = static_cast<int>(hs.disp.disp32);
            if (offset > flMVMLastDamageTimeOffset && offset < bInPowerPlayOffset)
            {
                playerObjectsCountOffset = offset;
                break;
            }
        }
        instr += hs.len;
    }
    assert(playerObjectsCountOffset > 0);
    sPlayerObjectsOffset = playerObjectsCountOffset - offsetof(CUtlVector<CBaseHandle>, m_Size);
    assert(sPlayerObjectsOffset > 0);

    // from CTFPlayer
    struct TFPlayerObservableEntitiesHack
    {
        CUtlVector<EHANDLE>	m_aObjects; // List of player objects
        bool m_bIsClassMenuOpen;
        Vector m_vecLastDeathPosition;
        float m_flSpawnTime;
        float m_flLastAction;
        float m_flTimeInSpawn;
        CUtlVector<EHANDLE>	m_hObservableEntities;
    };
    sObservableEntitiesOffset = sPlayerObjectsOffset + offsetof(TFPlayerObservableEntitiesHack, m_hObservableEntities);
    assert(sObservableEntitiesOffset > 0);
}

int TFBaseRocketHelpers::sLauncherOffset;

void TFBaseRocketHelpers::InitializeOffsets(CBaseEntity* ent)
{
    if (sLauncherOffset > 0)
    {
        // already initialized
        return;
    }

    SendTable* table = EntityHelpers::GetTable(ent->GetServerClass(), "DT_TFBaseRocket");
    assert(table);

    {
        SendProp* prop = EntityHelpers::GetProp(table, "m_hLauncher");
        assert(prop);
        sLauncherOffset = prop->GetOffset();
        assert(sLauncherOffset > 0);
    }
}

int TFProjectileRocketHelpers::sCriticalOffset;

void TFProjectileRocketHelpers::InitializeOffsets(CBaseEntity* ent)
{
    if (sCriticalOffset > 0)
    {
        // already initialized
        return;
    }

    SendTable* table = EntityHelpers::GetTable(ent->GetServerClass(), "DT_TFProjectile_Rocket");
    assert(table);

    {
        SendProp* prop = EntityHelpers::GetProp(table, "m_bCritical");
        assert(prop);
        sCriticalOffset = prop->GetOffset();
        assert(sCriticalOffset > 0);
    }
}

int TFDroppedWeaponHelpers::sItemOffset;

void TFDroppedWeaponHelpers::InitializeOffsets(CBaseEntity* ent)
{
    if (sItemOffset > 0)
    {
        // already initialized
        return;
    }

    SendProp* prop = EntityHelpers::GetProp(ent->GetServerClass(), "DT_TFDroppedWeapon", "m_Item");
    assert(prop);

    sItemOffset = prop->GetOffset();
    assert(sItemOffset > 0);
}

int AttributeContainerHelpers::sItemOffset;

void AttributeContainerHelpers::InitializeOffsets(CAttributeContainer* container)
{
    if (sItemOffset > 0)
    {
        // already initialized
        return;
    }

    datamap_t* datamap = container->GetDataDescMap();
    assert(datamap);

    sItemOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_Item");
    assert(sItemOffset > 0);
}

int BaseTriggerHelpers::sDisabledOffset;
int BaseTriggerHelpers::sTouchingEntitiesOffset;

void BaseTriggerHelpers::InitializeOffsets(CBaseEntity* baseTrigger)
{
    if (sDisabledOffset > 0)
    {
        // already initialized
        return;
    }

    datamap_t* datamap = baseTrigger->GetDataDescMap();
    assert(datamap);

    sDisabledOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_bDisabled");
    assert(sDisabledOffset > 0);

    sTouchingEntitiesOffset = EntityHelpers::GetDatamapVarOffset(datamap, "m_hTouchingEntities");
    assert(sTouchingEntitiesOffset > 0);
}

int TeamplayRoundBasedRulesHelpers::sRoundStateOffset;

void TeamplayRoundBasedRulesHelpers::InitializeOffsets(IServerGameDLL* serverGameDll)
{
    if (sRoundStateOffset > 0)
    {
        // already initialized
        return;
    }

    SendProp* prop = EntityHelpers::GetProp(serverGameDll, "CTeamplayRoundBasedRulesProxy", "DT_TeamplayRoundBasedRules", "m_iRoundState");
    assert(prop);

    sRoundStateOffset = prop->GetOffset();
    assert(sRoundStateOffset > 0);
}
