
#include "SizzLauncherSpawner.h"

#include "sourcesdk/game/server/baseentity.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/public/toolframework/itoolentity.h"
#include "sourcesdk/public/basehandle.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/icvar.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/game/server/iplayerinfo.h"
#include "sourcesdk/game/shared/econ/econ_item_view.h"
#include "sourcesdk/game/shared/econ/ihasattributes.h"
#include "sourcesdk/game/shared/econ/attribute_manager.h"
#include "sourcesdk/game/shared/econ/econ_item_constants.h"
#include "sourcesdk/game/shared/teamplayroundbased_gamerules.h"

#include "sourcehelpers/EntityHelpers.h"
#include "sourcehelpers/VStdlibRandom.h"
#include "HookOffsets.h"
#include "SizzLauncherInfo.h"

#include <string.h>

VTableHook<decltype(&SizzLauncherSpawner::RocketLauncherSpawnHook)> SizzLauncherSpawner::sRocketLauncherSpawnHook;
VTableHook<decltype(&SizzLauncherSpawner::DroppedWeaponSpawnHook)> SizzLauncherSpawner::sDroppedWeaponSpawnHook;

SizzLauncherSpawner::SizzLauncherSpawner() :
    mServerGameClients(nullptr),
    mServerTools(nullptr),
    mPlayerInfoManager(nullptr),
    mVEngineServer(nullptr),
    mServer(nullptr),
    mGlobals(nullptr),
    mGameRules(nullptr),
    mTfDroppedWeaponLifetime(nullptr),
    mInitialSpawnIntervalTicks(0),
    mSpawnIntervalTicks(0),
    mNextSpawnTick(0),
    mRoundState(0)
{
}

SizzLauncherSpawner::~SizzLauncherSpawner()
{
}

bool SizzLauncherSpawner::Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    mServerGameClients = (IServerGameClients*)gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, nullptr);
    mServerTools = (IServerTools*)gameServerFactory(VSERVERTOOLS_INTERFACE_VERSION, nullptr);
    mVEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    if (mVEngineServer)
    {
        mServer = mVEngineServer->GetIServer();
    }
    ICvar* cvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr);

    mPlayerInfoManager = (IPlayerInfoManager*)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, nullptr);
    if (mPlayerInfoManager)
    {
        mGlobals = mPlayerInfoManager->GetGlobalVars();
    }

    if (!mServerGameClients || !mServerTools || !mServer || !cvar || !mGlobals)// || !mServerGameDll || !mServerGameEnts || !mVEngineServer)
    {
        return false;
    }

    mTfDroppedWeaponLifetime = cvar->FindVar("tf_dropped_weapon_lifetime");
    if (!mTfDroppedWeaponLifetime)
    {
        return false;
    }

    return true;
}

void SizzLauncherSpawner::Shutdown()
{
    sDroppedWeaponSpawnHook.Unhook();
    sRocketLauncherSpawnHook.Unhook();
}

void SizzLauncherSpawner::LevelInit(const char* pMapName)
{
    mInitialSpawnIntervalTicks = static_cast<uint32_t>(sInitialSpawnIntervalSeconds / mGlobals->interval_per_tick);
    mSpawnIntervalTicks = static_cast<uint32_t>(sSpawnIntervalSeconds / mGlobals->interval_per_tick);

    CBaseEntity* ent = mServerTools->CreateEntityByName("tf_weapon_rocketlauncher");
    if (ent)
    {
        IHasAttributes* attributeInterface = BaseEntityHelpers::GetAttribInterface(ent);
        assert(attributeInterface);
        CAttributeContainer* con = attributeInterface->GetAttributeContainer();
        assert(con);

        AttributeContainerHelpers::InitializeOffsets(con);

        if (!sRocketLauncherSpawnHook.GetThisPtr())
        {
            sRocketLauncherSpawnHook.Hook(ent, HookOffsets::Spawn, this, &SizzLauncherSpawner::RocketLauncherSpawnHook);
        }
        mServerTools->RemoveEntityImmediate(ent);
    }
    CBaseEntity* droppedWeapon = mServerTools->CreateEntityByName("tf_dropped_weapon");
    if (droppedWeapon)
    {
        TFDroppedWeaponHelpers::InitializeOffsets(droppedWeapon);
        if (!sDroppedWeaponSpawnHook.GetThisPtr())
        {
            sDroppedWeaponSpawnHook.Hook(droppedWeapon, HookOffsets::Spawn, this, &SizzLauncherSpawner::DroppedWeaponSpawnHook);
        }
        mServerTools->RemoveEntityImmediate(droppedWeapon);
    }
}

void SizzLauncherSpawner::ServerActivate(CGameRules* gameRules)
{
    mGameRules = gameRules;

    CTeamplayRoundBasedRules* rules = reinterpret_cast<CTeamplayRoundBasedRules*>(mGameRules);
    mRoundState = TeamplayRoundBasedRulesHelpers::GetRoundState(rules);
}

void SizzLauncherSpawner::GameFrme(bool bSimulating)
{
    if (!bSimulating)
    {
        return;
    }

    if (mGameRules)
    {
        CTeamplayRoundBasedRules* rules = reinterpret_cast<CTeamplayRoundBasedRules*>(mGameRules);
        const int roundState = TeamplayRoundBasedRulesHelpers::GetRoundState(rules);
        bool roundStateChanged = false;
        if (mRoundState != roundState)
        {
            roundStateChanged = true;
            mRoundState = roundState;
        }

        if (roundState == GR_STATE_RND_RUNNING)
        {
            const uint32_t curTick = mGlobals->tickcount;
            if (roundStateChanged)
            {
                mNextSpawnTick = curTick + mInitialSpawnIntervalTicks;
            }

            if (curTick >= mNextSpawnTick)
            {
                mNextSpawnTick = curTick + mSpawnIntervalTicks;

                uint8_t elgibleSpawnPoints[MAX_PLAYERS];
                int numElgibleSpawnPoints = 0;

                const int numClients = mServer->GetClientCount();
                for (int i = 0; i < numClients; ++i)
                {
                    const int entIndex = i + 1;
                    edict_t* edict = mVEngineServer->PEntityOfEntIndex(entIndex);
                    if (!edict)
                    {
                        continue;
                    }

                    IPlayerInfo* playerInfo = mPlayerInfoManager->GetPlayerInfo(edict);
                    if (playerInfo &&
                        !playerInfo->IsHLTV() && !playerInfo->IsReplay() &&
                        !playerInfo->IsDead() && !playerInfo->IsObserver())
                    {
                        elgibleSpawnPoints[numElgibleSpawnPoints++] = entIndex;
                    }
                }

                if (numElgibleSpawnPoints > 0)
                {
                    const int spawnPointIndex = VStdlibRandom::RandomInt(0, numElgibleSpawnPoints - 1);
                    const int spawnPointEnt = elgibleSpawnPoints[spawnPointIndex];

                    edict_t* edict = mVEngineServer->PEntityOfEntIndex(spawnPointEnt);

                    Vector origin;
                    mServerGameClients->ClientEarPosition(edict, &origin);
                    SpawnLauncher(origin);
                }
            }
        }
    }
}

void SizzLauncherSpawner::LevelShutdown()
{
    mGameRules = nullptr;
}

struct CTFDroppedWeapon_Hack
{
    struct alignas(CEconItemView) CEconItemView_Hack
    {
        char buf[sizeof(CEconItemView)];
    };

    CEconItemView_Hack m_Item;
    float m_flChargeLevel;

    CBaseHandle m_hPlayer;
    int m_nClip;
    int m_nAmmo;
    int m_nDetonated;
    float m_flEnergy;
    float m_flEffectBarRegenTime;
    float m_flNextPrimaryAttack;
    float m_flNextSecondaryAttack;
};

static void ApplyFestiveRocketLauncher(CBaseEntity* ent, IServerTools* serverTools)
{
    serverTools->SetKeyValue(ent, "targetname", "models/weapons/c_models/c_rocketlauncher/c_rocketlauncher.mdl");
    string_t modelName = BaseEntityHelpers::GetName(ent);
    BaseEntityHelpers::SetModelName(ent, modelName);
    serverTools->SetKeyValue(ent, "targetname", "");

    TFDroppedWeaponHelpers::InitializeOffsets(ent);
    CEconItemView& item = TFDroppedWeaponHelpers::GetItem(ent);

    CTFDroppedWeapon_Hack* tfDroppedWeapon = (CTFDroppedWeapon_Hack*)&item;
    tfDroppedWeapon->m_nClip = SizzLauncherInfo::InitialClip;
    tfDroppedWeapon->m_nAmmo = SizzLauncherInfo::InitialAmmoReserves;

    // my festive rocket launcher
    item.m_iItemDefinitionIndex = SizzLauncherInfo::ItemDefinitionIndex;
    item.m_iEntityQuality = SizzLauncherInfo::EntityQuality;
    item.m_iEntityLevel = SizzLauncherInfo::EntityLevel;
    item.m_iItemID = SizzLauncherInfo::ItemID;
    item.m_iItemIDHigh = SizzLauncherInfo::ItemIDHigh;
    item.m_iItemIDLow = SizzLauncherInfo::ItemIDLow;
    item.m_iAccountID = SizzLauncherInfo::AccountID;
    item.m_iInventoryPosition = 0;// 2147484044;
    item.m_pNonSOEconItem.m_iItemID = -1;
    item.m_pNonSOEconItem.m_pItem = nullptr;
    item.m_pNonSOEconItem.m_OwnerSteamID = 0;
    item.m_bColorInit = false;
    item.m_bPaintOverrideInit = false;
    item.m_bHasPaintOverride = false;
    item.m_flOverrideIndex = 0.0f;
    item.m_unRGB = 0;
    item.m_unAltRGB = 0;
    item.m_iTeamNumber = 1; // spectate
    item.m_bInitialized = true;
    item.m_bOnlyIterateItemViewAttributes = false;

    // attributes are set after weapon spawn because CTFWeaponBase::Spawn
    // calls InitializeAttributes which calls InitNetworkedDynamicAttributesForDemos.
    // That overwrites everything.
}

bool SizzLauncherSpawner::ClientCommand(edict_t* pEntity, const CCommand& args)
{
    const char* command = args.Arg(0);
    if (!strcmp(command, "spawnlauncher"))
    {
        Vector origin;
        mServerGameClients->ClientEarPosition(pEntity, &origin);

        SpawnLauncher(origin);
        return true;
    }

    return false;
}

CBaseHandle SizzLauncherSpawner::SpawnLauncher(const Vector& origin)
{
    CBaseEntity* ent = mServerTools->CreateEntityByName("tf_dropped_weapon");
    if (ent)
    {
        ApplyFestiveRocketLauncher(ent, mServerTools);

        mServerTools->SetKeyValue(ent, "origin", origin);
        mServerTools->DispatchSpawn(ent);
        return ent->GetRefEHandle();
    }
    return CBaseHandle();
}

void SizzLauncherSpawner::RocketLauncherSpawnHook()
{
    SizzLauncherSpawner* thisPtr = sRocketLauncherSpawnHook.GetThisPtr();
    CBaseEntity* rocketLauncher = reinterpret_cast<CBaseEntity*>(this);
    sRocketLauncherSpawnHook.CallOriginalFn(this);
    thisPtr->RocketLauncherSpawn(rocketLauncher);
}

void SizzLauncherSpawner::RocketLauncherSpawn(CBaseEntity* rocketLauncher)
{
    IHasAttributes* attributeInterface = BaseEntityHelpers::GetAttribInterface(rocketLauncher);
    assert(attributeInterface);
    CAttributeContainer* con = attributeInterface->GetAttributeContainer();
    assert(con);

    CEconItemView& item = AttributeContainerHelpers::GetItem(con);
    if (item.m_iItemID != SizzLauncherInfo::ItemID)
    {
        return;
    }

    static void* CEconItemAttribute_vtable = nullptr;
    if (!CEconItemAttribute_vtable)
    {
        CUtlVector<CEconItemAttribute>& attributes = item.m_NetworkedDynamicAttributesForDemos.m_Attributes;
        if (attributes.Count() > 0)
        {
            CEconItemAttribute_vtable = *(void**)(attributes.Base());
        }
    }

    // https://wiki.teamfortress.com/wiki/List_of_item_attributes
    // Or check items_game.txt
    const CEconItemAttribute attrs[] =
    {
        { CEconItemAttribute_vtable, 2, 1.5f },     // mult_dmg - 1.5x damage
        { CEconItemAttribute_vtable, 3, 0.25f },    // mult_clipsize - clip size to 1
        { CEconItemAttribute_vtable, 77, 0.5f },    // mult_maxammo_primary - 0.5x max primary ammo
        { CEconItemAttribute_vtable, 99, 2.0f },    // mult_explosion_radius - blast radius multiplier
        //{ CEconItemAttribute_vtable, 118, 0.5f },   // mult_dmg_falloff - splash damage removal
        { CEconItemAttribute_vtable, 488, 1.0f },   // rocket_specialist - +15% rocket speed per point. On direct hits: rocket does maximum damage, stuns target, and blast radius increased +15% per point.
        //{ CEconItemAttribute_vtable, 521, 1.0f },   // use_large_smoke_explosion - sentrybuster explosion
        { CEconItemAttribute_vtable, 104, 0.5f },   // mult_projectile_speed - 0.5x rocket speed
    };

    CUtlVector<CEconItemAttribute>* attrLists[] =
    {
        &item.m_AttributeList.m_Attributes,
        &item.m_NetworkedDynamicAttributesForDemos.m_Attributes
    };

    /*Debug::Msg("attributes: %s\n", BaseEntityHelpers::GetClassname(rocketLauncher));
    for (CUtlVector<CEconItemAttribute>* attributes : attrLists)
    {
        for (int i = 0; i < attributes->Count(); ++i)
        {
            auto& attr = attributes->Element(i);
            Debug::Msg("%i %f\n", attr.m_iAttributeDefinitionIndex, attr.m_flValue);
        }
        Debug::Msg("\n");
    }*/

    for (CUtlVector<CEconItemAttribute>* attributes : attrLists)
    {
        for (const CEconItemAttribute& attr : attrs)
        {
            const int index = attributes->Find(attr);
            if (index >= 0)
            {
                CEconItemAttribute& existingAttr = attributes->Element(index);
                existingAttr.m_flValue = attr.m_flValue;
            }
            else
            {
                attributes->AddToTail(attr);
            }
        }
    }

    con->OnAttributeValuesChanged();
}

void SizzLauncherSpawner::DroppedWeaponSpawnHook()
{
    SizzLauncherSpawner* thisPtr = sDroppedWeaponSpawnHook.GetThisPtr();
    CBaseEntity* droppedWeapon = reinterpret_cast<CBaseEntity*>(this);
    thisPtr->DroppedWeaponSpawn(droppedWeapon);
}

void SizzLauncherSpawner::DroppedWeaponSpawn(CBaseEntity* droppedWeapon)
{
    assert(mTfDroppedWeaponLifetime);

    const float oldLifetime = mTfDroppedWeaponLifetime->m_pParent->m_fValue;

    TFDroppedWeaponHelpers::InitializeOffsets(droppedWeapon);
    CEconItemView& item = TFDroppedWeaponHelpers::GetItem(droppedWeapon);
    if (item.m_iItemID == SizzLauncherInfo::ItemID)
    {
        mTfDroppedWeaponLifetime->m_pParent->m_fValue = 60.0f;
    }
    sDroppedWeaponSpawnHook.CallOriginalFn(reinterpret_cast<SizzLauncherSpawner*>(droppedWeapon));

    mTfDroppedWeaponLifetime->m_pParent->m_fValue = oldLifetime;
}
