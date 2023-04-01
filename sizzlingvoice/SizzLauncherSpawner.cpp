
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
#include "sourcesdk/public/tier1/bitbuf.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/game/server/iplayerinfo.h"
#include "sourcesdk/game/shared/econ/econ_item_view.h"
#include "sourcesdk/game/shared/econ/ihasattributes.h"
#include "sourcesdk/game/shared/econ/attribute_manager.h"
#include "sourcesdk/game/shared/econ/econ_item_constants.h"
#include "sourcesdk/game/shared/teamplayroundbased_gamerules.h"

#include "base/math.h"
#include "sourcehelpers/CVarHelper.h"
#include "sourcehelpers/EntityHelpers.h"
#include "sourcehelpers/RecipientFilter.h"
#include "sourcehelpers/VStdlibRandom.h"
#include "HookOffsets.h"
#include "SizzLauncherInfo.h"
#include "Version.h"

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
    mSpawnInitialDelay(nullptr),
    mSpawnInterval(nullptr),
    mSpawnsEnabled(nullptr),
    mSpawnCommandEnabled(nullptr),
    mRocketDamageMultiplier(nullptr),
    mRocketMaxAmmoMult(nullptr),
    mRocketSpecialistEnabled(nullptr),
    mRocketSpeedMultiplier(nullptr),
    mLauncherPickupLifetime(nullptr),
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

    ConVar* hudHintSound = cvar->FindVar("sv_hudhint_sound");
    if (hudHintSound)
    {
        hudHintSound->SetValue(0);
    }

    mSpawnInitialDelay = CVarHelper::CreateConVar("sizz_rocketmode_spawn_initialdelay", "15.0f", "Delay before spawning the first set of launchers after round start (seconds).");
    mSpawnInterval = CVarHelper::CreateConVar("sizz_rocketmode_spawn_interval", "60.0f", "After initial spawns, periodically spawn launchers on this interval (seconds).");
    mSpawnsEnabled = CVarHelper::CreateConVar("sizz_rocketmode_spawn_enabled", "1", "Enables periodic spawning of rocket mode rocket launchers (0/1).");
    mSpawnCommandEnabled = CVarHelper::CreateConVar("sizz_rocketmode_spawn_command_enabled", "0", "Enables clients to spawn a launcher at their feet with 'spawnlauncher' (0/1)");
    mRocketDamageMultiplier = CVarHelper::CreateConVar("sizz_rocketmode_damagemult", "1.5f", "Damage multiplier over base rocket damage [0.1f, 10.0f].");
    mRocketMaxAmmoMult = CVarHelper::CreateConVar("sizz_rocketmode_ammomult", "1.0f", "Reserve ammo multiplier over base rocket launcher [0.1f, 1.0f].");
    mRocketSpecialistEnabled = CVarHelper::CreateConVar("sizz_rocketmode_rocketspecialist", "1", "Enables the Rocket Specialist attribute on sizz launchers (0/1).");
    mRocketSpeedMultiplier = CVarHelper::CreateConVar("sizz_rocketmode_speedmult", "0.5f", "Speed multiplier over base rocket speed [0.1f, 10.0f].");
    mLauncherPickupLifetime = CVarHelper::CreateConVar("sizz_rocketmode_pickup_lifetime", "60.0f", "tf_dropped_weapon_lifetime, but specific to sizz launchers (seconds).");
    
    return true;
}

void SizzLauncherSpawner::Shutdown()
{
    CVarHelper::DestroyConVar(mLauncherPickupLifetime);
    CVarHelper::DestroyConVar(mRocketSpeedMultiplier);
    CVarHelper::DestroyConVar(mRocketSpecialistEnabled);
    CVarHelper::DestroyConVar(mRocketMaxAmmoMult);
    CVarHelper::DestroyConVar(mRocketDamageMultiplier);
    CVarHelper::DestroyConVar(mSpawnCommandEnabled);
    CVarHelper::DestroyConVar(mSpawnsEnabled);
    CVarHelper::DestroyConVar(mSpawnInterval);
    CVarHelper::DestroyConVar(mSpawnInitialDelay);
    sDroppedWeaponSpawnHook.Unhook();
    sRocketLauncherSpawnHook.Unhook();
}

void SizzLauncherSpawner::LevelInit(const char* pMapName)
{
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
    if (!bSimulating || !mSpawnsEnabled->IsEnabled())
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
                const uint32_t initialDelay = static_cast<uint32_t>(mSpawnInitialDelay->GetFloat() / mGlobals->interval_per_tick);
                mNextSpawnTick = curTick + initialDelay;
            }

            if (curTick >= mNextSpawnTick)
            {
                const uint32_t spawnInterval = static_cast<uint32_t>(mSpawnInterval->GetFloat() / mGlobals->interval_per_tick);
                mNextSpawnTick = curTick + spawnInterval;

                uint8_t redSpawnPoints[MAX_PLAYERS];
                uint8_t bluSpawnPoints[MAX_PLAYERS];
                int numRedSpawnPoints = 0;
                int numBluSpawnPoints = 0;

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
                        int team = playerInfo->GetTeamIndex();
                        if (team == 2) // red
                        {
                            redSpawnPoints[numRedSpawnPoints++] = entIndex;
                        }
                        else if (team == 3) // blu
                        {
                            bluSpawnPoints[numBluSpawnPoints++] = entIndex;
                        }
                    }
                }

                if (numRedSpawnPoints || numBluSpawnPoints)
                {
                    RecipientFilter filter;
                    filter.AddAllPlayers(mServer);

                    bf_write* buf = mVEngineServer->UserMessageBegin(&filter, 3); // SayText
                    buf->WriteByte(0);
                    buf->WriteString("\x05========================\n\x03" CREDITS "\n\x05========================\n");
                    buf->WriteByte(0);
                    mVEngineServer->MessageEnd();
                }
                if (numRedSpawnPoints > 0)
                {
                    const int spawnPointIndex = VStdlibRandom::RandomInt(0, numRedSpawnPoints - 1);
                    const int spawnPointEnt = redSpawnPoints[spawnPointIndex];

                    edict_t* edict = mVEngineServer->PEntityOfEntIndex(spawnPointEnt);

                    Vector origin;
                    mServerGameClients->ClientEarPosition(edict, &origin);
                    SpawnLauncher(origin);
                }
                if (numBluSpawnPoints > 0)
                {
                    const int spawnPointIndex = VStdlibRandom::RandomInt(0, numBluSpawnPoints - 1);
                    const int spawnPointEnt = bluSpawnPoints[spawnPointIndex];

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

void SizzLauncherSpawner::ApplyFestiveRocketLauncher(CBaseEntity* ent, IServerTools* serverTools)
{
    serverTools->SetKeyValue(ent, "targetname", "models/weapons/c_models/c_rocketlauncher/c_rocketlauncher.mdl");
    string_t modelName = BaseEntityHelpers::GetName(ent);
    BaseEntityHelpers::SetModelName(ent, modelName);
    serverTools->SetKeyValue(ent, "targetname", "");

    TFDroppedWeaponHelpers::InitializeOffsets(ent);
    CEconItemView& item = TFDroppedWeaponHelpers::GetItem(ent);

    const float maxAmmoMult = Math::Clamp(mRocketMaxAmmoMult->GetFloat(), 0.1f, 1.0f);

    CTFDroppedWeapon_Hack* tfDroppedWeapon = (CTFDroppedWeapon_Hack*)&item;
    tfDroppedWeapon->m_nClip = SizzLauncherInfo::InitialClip;
    tfDroppedWeapon->m_nAmmo = SizzLauncherInfo::InitialAmmoReserves * maxAmmoMult;

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
    if (mSpawnCommandEnabled->IsEnabled())
    {
        const char* command = args.Arg(0);
        if (!strcmp(command, "spawnlauncher"))
        {
            Vector origin;
            mServerGameClients->ClientEarPosition(pEntity, &origin);

            SpawnLauncher(origin);
            return true;
        }
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

    const float rocketDamageMult = Math::Clamp(mRocketDamageMultiplier->GetFloat(), 0.1f, 10.0f);
    const float maxAmmoMult = Math::Clamp(mRocketMaxAmmoMult->GetFloat(), 0.1f, 1.0f);
    const float rocketSpecialist = mRocketSpecialistEnabled->IsEnabled() ? 1.0f : 0.0f;
    const float rocketSpeedMult = Math::Clamp(mRocketSpeedMultiplier->GetFloat(), 0.1f, 10.0f);

    // https://wiki.teamfortress.com/wiki/List_of_item_attributes
    // Or check items_game.txt
    const CEconItemAttribute attrs[] =
    {
        { CEconItemAttribute_vtable, 2, rocketDamageMult },     // mult_dmg - 1.5x damage
        { CEconItemAttribute_vtable, 3, 0.25f },    // mult_clipsize - clip size to 1
        { CEconItemAttribute_vtable, 77, maxAmmoMult },    // mult_maxammo_primary - 0.5x max primary ammo
        { CEconItemAttribute_vtable, 99, 2.0f },    // mult_explosion_radius - blast radius multiplier
        //{ CEconItemAttribute_vtable, 118, 0.5f },   // mult_dmg_falloff - splash damage removal
        { CEconItemAttribute_vtable, 488, rocketSpecialist },   // rocket_specialist - +15% rocket speed per point. On direct hits: rocket does maximum damage, stuns target, and blast radius increased +15% per point.
        //{ CEconItemAttribute_vtable, 521, 1.0f },   // use_large_smoke_explosion - sentrybuster explosion
        { CEconItemAttribute_vtable, 104, rocketSpeedMult },   // mult_projectile_speed - 0.5x rocket speed

        // these are from the m_NetworkedDynamicAttributesForDemos that we have to clear.
        { CEconItemAttribute_vtable, 2025, 1.0f },  // killstreak_tier
        { CEconItemAttribute_vtable, 724, 1.0f },   // weapon_stattrak_module_scale 1
        { CEconItemAttribute_vtable, 692, 0.0f }    // limited_quantity_item
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

    CUtlVector<CEconItemAttribute>* attributes = &item.m_AttributeList.m_Attributes;
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
    // To get min viewmodels working, we need
    // m_NetworkedDynamicAttributesForDemos to be empty.
    // The game will query static data in that case which has the offset string.
    item.m_NetworkedDynamicAttributesForDemos.m_Attributes.RemoveAll();

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
        mTfDroppedWeaponLifetime->m_pParent->m_fValue = mLauncherPickupLifetime->GetFloat();
    }
    sDroppedWeaponSpawnHook.CallOriginalFn(reinterpret_cast<SizzLauncherSpawner*>(droppedWeapon));

    mTfDroppedWeaponLifetime->m_pParent->m_fValue = oldLifetime;
}
