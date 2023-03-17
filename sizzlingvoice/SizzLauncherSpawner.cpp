
#include "SizzLauncherSpawner.h"

#include "sourcesdk/game/server/baseentity.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/public/toolframework/itoolentity.h"
#include "sourcesdk/public/basehandle.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/game/shared/econ/econ_item_view.h"
#include "sourcesdk/game/shared/econ/ihasattributes.h"
#include "sourcesdk/game/shared/econ/attribute_manager.h"
#include "sourcesdk/game/shared/econ/econ_item_constants.h"

#include "sourcehelpers/EntityHelpers.h"

#include <string.h>

VTableHook<decltype(&SizzLauncherSpawner::RocketLauncherSpawnHook)> SizzLauncherSpawner::sRocketLauncherSpawnHook;

SizzLauncherSpawner::SizzLauncherSpawner() :
    mServerGameClients(nullptr),
    mServerTools(nullptr)
    //mServerGameDll(nullptr),
    //mServerGameEnts(nullptr),
    //mVEngineServer(nullptr)
{
}

SizzLauncherSpawner::~SizzLauncherSpawner()
{
}

bool SizzLauncherSpawner::Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    mServerGameClients = (IServerGameClients*)gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, nullptr);
    mServerTools = (IServerTools*)gameServerFactory(VSERVERTOOLS_INTERFACE_VERSION, nullptr);
    //mServerGameDll = (IServerGameDLL*)gameServerFactory(INTERFACEVERSION_SERVERGAMEDLL, nullptr);
    //mServerGameEnts = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, nullptr);
    //mVEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    if (!mServerGameClients || !mServerTools)// || !mServerGameDll || !mServerGameEnts || !mVEngineServer)
    {
        return false;
    }

    return true;
}

void SizzLauncherSpawner::Shutdown()
{
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
#ifdef SDK_COMPAT
            constexpr int Offset = 22;
#else
            constexpr int Offset = 24;
#endif
            sRocketLauncherSpawnHook.Hook(ent, Offset, this, &SizzLauncherSpawner::RocketLauncherSpawnHook);
        }
        mServerTools->RemoveEntityImmediate(ent);
    }
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

    SendProp* itemProp = EntityHelpers::GetProp(ent->GetServerClass(), "DT_TFDroppedWeapon", "m_Item");
    const int itemOffset = itemProp->GetOffset();

    CEconItemView& item = TFDroppedWeaponHelpers::GetItem(ent);

    CTFDroppedWeapon_Hack* tfDroppedWeapon = (CTFDroppedWeapon_Hack*)&item;
    tfDroppedWeapon->m_nClip = 4;
    tfDroppedWeapon->m_nAmmo = 20;

    // my festive rocket launcher
    item.m_iItemDefinitionIndex = 658;
    item.m_iEntityQuality = EEconItemQuality::AE_UNIQUE;
    item.m_iEntityLevel = 1;
    item.m_iItemID = 3977757014;
    item.m_iItemIDHigh = 0;
    item.m_iItemIDLow = 3977757014;
    item.m_iAccountID = 28707326;
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
    if (!strcmp(command, "spawn") && (args.ArgC() > 1))
    {
        CBaseEntity* ent = mServerTools->CreateEntityByName("tf_dropped_weapon");
        if (ent)
        {
            TFDroppedWeaponHelpers::InitializeOffsets(ent);

            if (!strcmp(args.Arg(1), "launcher"))
            {
                ApplyFestiveRocketLauncher(ent, mServerTools);
            }

            Vector origin;
            mServerGameClients->ClientEarPosition(pEntity, &origin);
            mServerTools->SetKeyValue(ent, "origin", origin);

            mServerTools->DispatchSpawn(ent);
        }
        return true;
    }

    return false;
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
    if (item.m_iItemID != 3977757014)
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
        { CEconItemAttribute_vtable, 2, 2.0f }, // mult_dmg - 2x damage
        { CEconItemAttribute_vtable, 3, 0.25f }, // mult_clipsize - clip size to 1
        //{ CEconItemAttribute_vtable, 118, 0.5f }, // mult_dmg_falloff - splash damage removal
        { CEconItemAttribute_vtable, 488, 1.0f }, // rocket_specialist - +15% rocket speed per point. On direct hits: rocket does maximum damage, stuns target, and blast radius increased +15% per point.
        { CEconItemAttribute_vtable, 521, 1.0f },  // use_large_smoke_explosion - sentrybuster explosion
        { CEconItemAttribute_vtable, 104, 0.5f },  // mult_projectile_speed - halve rocket speed
    };

    CUtlVector<CEconItemAttribute>* attrLists[] =
    {
        &item.m_AttributeList.m_Attributes,
        &item.m_NetworkedDynamicAttributesForDemos.m_Attributes
    };

    for (CUtlVector<CEconItemAttribute>* attributes : attrLists)
    {
        for (const CEconItemAttribute& attr : attrs)
        {
            if (attributes->Find(attr) < 0)
            {
                attributes->AddToTail(attr);
            }
        }
    }

    con->OnAttributeValuesChanged();
}
