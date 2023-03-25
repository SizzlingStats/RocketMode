
#include "RocketMode.h"

#include "sourcehelpers/ClientHelpers.h"
#include "sourcehelpers/EntityHelpers.h"

#include "sourcesdk/game/server/baseentity.h"
#include "sourcesdk/game/server/iplayerinfo.h"
#include "sourcesdk/game/shared/econ/econ_item_view.h"
#include "sourcesdk/game/shared/econ/ihasattributes.h"
#include "sourcesdk/game/shared/in_buttons.h"
#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/game/shared/usercmd.h"
#include "sourcesdk/public/engine/IEngineSound.h"
#include "sourcesdk/public/basehandle.h"
#include "sourcesdk/public/bspflags.h"
#include "sourcesdk/public/const.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/gametrace.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/icvar.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/mathlib/mathlib.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/public/toolframework/itoolentity.h"

#include "sourcehelpers/RecipientFilter.h"
#include "sourcehelpers/SendTablesFix.h"
#include "sourcehelpers/Vector.h"
#include "base/math.h"
#include "HookOffsets.h"
#include <string.h>

#define BOOSTER_LOOP "ambient/steam_drum.wav"
#define CRIT_LOOP "weapons/crit_power.wav"

string_t RocketMode::tf_projectile_rocket;
VTableHook<decltype(&RocketMode::PlayerRunCommandHook)> RocketMode::sPlayerRunCommandHook;
VTableHook<decltype(&RocketMode::SetOwnerEntityHook)> RocketMode::sSetOwnerEntityHook;
VTableHook<decltype(&RocketMode::RocketChangeTeamHook)> RocketMode::sRocketChangeTeamHook;

inline float VectorLength(const Vector& v)
{
    return Math::Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void AngleVectors(const QAngle& angles, Vector& forward)
{
    const float yaw = DegToRad(angles.y);
    const float pitch = DegToRad(angles.x);

    float sy, cy, sp, cp;
    Math::SinCos(yaw, &sy, &cy);
    Math::SinCos(pitch, &sp, &cp);

    forward.x = cp * cy;
    forward.y = cp * sy;
    forward.z = -sp;
}

RocketMode::RocketMode() :
    mVEngineServer(nullptr),
    mServer(nullptr),
    mServerTools(nullptr),
    mServerGameDll(nullptr),
    mCvar(nullptr),
    mServerGameEnts(nullptr),
    mGlobals(nullptr),
    mGameEventManager(nullptr),
    mEngineSound(nullptr),
    mSendTables(nullptr),
    mTFBaseRocketClass(nullptr),
    mGameRules(nullptr),
    mClientStates()
{
}

bool RocketMode::Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    mVEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    if (mVEngineServer)
    {
        mServer = mVEngineServer->GetIServer();
    }
    mServerTools = (IServerTools*)gameServerFactory(VSERVERTOOLS_INTERFACE_VERSION, nullptr);
    mServerGameDll = (IServerGameDLL*)gameServerFactory(INTERFACEVERSION_SERVERGAMEDLL, nullptr);
    mCvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr);
    mServerGameEnts = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, nullptr);

    IPlayerInfoManager* playerInfoManager = (IPlayerInfoManager*)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, nullptr);
    if (playerInfoManager)
    {
        mGlobals = playerInfoManager->GetGlobalVars();
    }
    mGameEventManager = (IGameEventManager2*)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
    mEngineSound = (IEngineSound*)interfaceFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, nullptr);

    if (!mVEngineServer || !mServer || !mServerTools || !mCvar || !mServerGameEnts || !mGlobals || !mGameEventManager || !mEngineSound)
    {
        return false;
    }

    if (!mGameEventManager->AddListener(this, "player_spawn", true))
    {
        return false;
    }
    if (!mGameEventManager->AddListener(this, "player_death", true))
    {
        return false;
    }

    if (!ModifyRocketAngularPrecision())
    {
        return false;
    }

    return true;
}

void RocketMode::Shutdown()
{
    if (mGameEventManager)
    {
        mGameEventManager->RemoveListener(this);
    }
    sRocketChangeTeamHook.Unhook();
    sSetOwnerEntityHook.Unhook();
    sPlayerRunCommandHook.Unhook();
}

void RocketMode::LevelInit(const char* pMapName)
{
    mEngineSound->PrecacheSound(BOOSTER_LOOP, true);
    mEngineSound->PrecacheSound(CRIT_LOOP, true);

    CBaseEntity* ent = mServerTools->CreateEntityByName("tf_projectile_rocket");
    if (ent)
    {
        TFBaseRocketHelpers::InitializeOffsets(ent);
        TFProjectileRocketHelpers::InitializeOffsets(ent);

        tf_projectile_rocket = BaseEntityHelpers::GetClassname(ent);
        if (!sSetOwnerEntityHook.GetThisPtr())
        {
            sSetOwnerEntityHook.Hook(ent, HookOffsets::SetOwnerEntity, this, &RocketMode::SetOwnerEntityHook);
        }
        if (!sRocketChangeTeamHook.GetThisPtr())
        {
            sRocketChangeTeamHook.Hook(ent, HookOffsets::ChangeTeam, this, &RocketMode::RocketChangeTeamHook);
        }

        mServerTools->RemoveEntityImmediate(ent);
    }
}

void RocketMode::ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
    SendProp* pSendProp = EntityHelpers::GetProp(mServerGameDll, "CTFGameRulesProxy", "DT_TeamplayRoundBasedRulesProxy", "teamplayroundbased_gamerules_data");
    if (pSendProp)
    {
        SendTableProxyFn proxyFn = pSendProp->GetDataTableProxyFn();
        if (proxyFn)
        {
            CSendProxyRecipients recp;
            mGameRules = reinterpret_cast<CGameRules*>(proxyFn(nullptr, nullptr, nullptr, &recp, 0));
        }
    }
    assert(mGameRules);
}

static int GetObserverTargetIndex(CBaseEntity* ent)
{
    if (ent)
    {
        CBaseHandle observerTarget = BasePlayerHelpers::GetObserverTarget(ent);
        const int targetEntIndex = observerTarget.GetEntryIndex();
        if (targetEntIndex <= MAX_PLAYERS)
        {
            return targetEntIndex;
        }
    }
    return 0;
}

void RocketMode::GameFrame(bool simulating)
{
    if (!simulating)
    {
        return;
    }

    const int clientCount = mServer->GetClientCount();

    // Rocket mode roll angle update
    for (int i = 0; i < clientCount; ++i)
    {
        CBaseHandle rocketHandle = mClientStates[i].rocket;
        if (!rocketHandle.IsValid())
        {
            // client is not in rocket mode
            continue;
        }

        CBaseEntity* rocketEnt = EntityHelpers::HandleToEnt(rocketHandle, mServerTools);
        if (!rocketEnt)
        {
            continue;
        }

        const QAngle& localAngVel = BaseEntityHelpers::GetLocalAngularVelocity(rocketEnt);

        // calculate new angRotation, but don't set it. PhysicsToss will do the same calculation as here.
        QAngle angRotation = BaseEntityHelpers::GetLocalRotation(rocketEnt);

        // Only write over z rotation (roll). The others will be computed by PhysicsToss with our localAngVel.
        angRotation.z = mClientStates[i].UpdateRoll(mGlobals->frametime, localAngVel.y * -0.16f); // 20 degrees of tilt
        //Debug::Msg("frame angRot %i: %f %f %f\n", mGlobals->framecount, angRotation.x, angRotation.y, angRotation.z);
        BaseEntityHelpers::SetLocalRotation(rocketEnt, angRotation);
    }

    // Spectator rocket mode update
    for (int i = 0; i < clientCount; ++i)
    {
        IClient* client = mServer->GetClient(i);
        if (!client || !client->IsActive())
        {
            continue;
        }

        const int clientEntIndex = i + 1;
        CBaseEntity* clientEnt = mServerTools->GetBaseEntityByEntIndex(clientEntIndex);
        if (!clientEnt)
        {
            // shouldn't happen, but for safety
            continue;
        }

        const int observerMode = BasePlayerHelpers::GetObserverMode(clientEnt);
        if (observerMode == OBS_MODE_NONE)
        {
            // player_spawn event notify handles spec -> respawn transitions.
            continue;
        }

        if (mClientStates[i].rocket.IsValid())
        {
            // client is in rocket mode
            continue;
        }

        edict_t* viewEdict = ClientHelpers::GetViewEntity(client);

        // if spectating a player
        if (observerMode == OBS_MODE_IN_EYE || observerMode == OBS_MODE_CHASE)
        {
            // OBS_MODE_CHASE seems to be set when on a fixed map view.
            // I thought it would be OBS_MODE_FIXED, but whatever.
            const int targetEntIndex = GetObserverTargetIndex(clientEnt);
            if (targetEntIndex > 0)
            {
                // if target is in rocket mode
                const int targetClientIndex = targetEntIndex - 1;
                const State& state = mClientStates[targetClientIndex];
                if (state.rocket.IsValid())
                {
                    // if target's rocket is not our view ent, set it.
                    CBaseEntity* rocketEnt = EntityHelpers::HandleToEnt(state.rocket, mServerTools);
                    edict_t* rocketEdict = mServerGameEnts->BaseEntityToEdict(rocketEnt);
                    if (viewEdict != rocketEdict)
                    {
                        ClientHelpers::SetViewEntity(client, rocketEdict);
                    }
                    continue;
                }
            }
        }

        if (viewEdict)
        {
            // Clear view ent and ent target if we're
            // not spectating anyone in rocket mode.
            // For spectator transitions.
            ClientHelpers::SetViewEntity(client, nullptr);
        }
    }
}

void RocketMode::LevelShutdown()
{
    mGameRules = nullptr;
    tf_projectile_rocket = nullptr;
}

void RocketMode::ClientConnect()
{
    assert(mSendTables);
    assert(mTFBaseRocketClass);
    assert(mVEngineServer);

    // m_FullSendTables construction happens right after ServerActivate.
    // It's overkill to reconstruct at each ClientConnect, but better than handling state.
    mSendTables->SetValue(1);
    SendTablesFix::ReconstructFullSendTablesForModification(mTFBaseRocketClass, mVEngineServer);
}

void RocketMode::ClientActive(edict_t* pEntity)
{
    if (!sPlayerRunCommandHook.GetThisPtr())
    {
        CBaseEntity* ent = mServerTools->GetBaseEntityByEntIndex(pEntity->m_EdictIndex);
        assert(ent);

        sPlayerRunCommandHook.Hook(ent, HookOffsets::PlayerRunCommand, this, &RocketMode::PlayerRunCommandHook);
    }
}

void RocketMode::ClientDisconnect(edict_t* pEntity)
{
    // When clients disconnect, their projectiles are instantly destroyed.
    // OnEntityDeleted will see a projectile with an owner edict marked FL_KILLME.

    // Clear out rocket and owner here just to make things less error prone.
    const int clientIndex = pEntity->m_EdictIndex - 1;
    assert(clientIndex < MAX_PLAYERS);
    mClientStates[clientIndex].Reset();
}

void RocketMode::OnEntityDeleted(CBaseEntity* pEntity)
{
    // if not "tf_projectile_rocket"
    const string_t classname = BaseEntityHelpers::GetClassname(pEntity);
    if (!tf_projectile_rocket || (classname != tf_projectile_rocket))
    {
        return;
    }

    edict_t* rocketEdict = mServerGameEnts->BaseEntityToEdict(pEntity);
    if (rocketEdict)
    {
        mEngineSound->StopSound(rocketEdict->m_EdictIndex, CHAN_WEAPON, BOOSTER_LOOP);
        mEngineSound->StopSound(rocketEdict->m_EdictIndex, CHAN_STATIC, CRIT_LOOP);
    }

    DetachFromRocket(pEntity);
}

void RocketMode::AttachToRocket(CBaseEntity* rocketEnt)
{
    // if not networked
    edict_t* edict = mServerGameEnts->BaseEntityToEdict(rocketEnt);
    if (!edict)
    {
        return;
    }

    // if no owner
    CBaseHandle ownerEntHandle = BaseEntityHelpers::GetOwnerEntity(rocketEnt);
    if (!ownerEntHandle.IsValid())
    {
        return;
    }

    // if owner isn't a player
    const int ownerEntIndex = ownerEntHandle.GetEntryIndex();
    if (ownerEntIndex <= 0 || ownerEntIndex >= MAX_PLAYERS)
    {
        return;
    }

    // if owner is a fake client (hltv, bot).
    const int ownerClientIndex = ownerEntIndex - 1;
    IClient* client = mServer->GetClient(ownerClientIndex);
    if (!client || client->IsFakeClient())
    {
        return;
    }

    CBaseHandle rocketHandle = rocketEnt->GetRefEHandle();
    State& state = mClientStates[ownerClientIndex];
    if (rocketHandle == state.rocket)
    {
        // already following this rocket.
        return;
    }

    {
        constexpr float collideWithTeammatesDelay = 0.25f;
        const SourceVector absVelocity = BaseEntityHelpers::GetAbsVelocity(rocketEnt);
        const SourceVector start = BaseEntityHelpers::GetAbsOrigin(rocketEnt);
        
        const Vector startVec = start;
        const Vector end = SourceVector(start + (absVelocity * collideWithTeammatesDelay));

        const int rocketTeam = BaseEntityHelpers::GetTeam(rocketEnt);
        const unsigned int mask = MASK_SOLID | ((rocketTeam == 2) ? CONTENTS_TEAM2 : CONTENTS_TEAM1 );

        trace_t trace;

        // float CGameRules::WeaponTraceEntity(CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd, unsigned int mask, trace_t *ptr);
        CallVFunc<float, CBaseEntity*, const Vector&, const Vector&, unsigned int, trace_t*>(
            HookOffsets::WeaponTraceEntity, mGameRules, rocketEnt, startVec, end, mask, &trace);

        // Rocket will likely hit something within 0.25s.
        // Avoid entering rocket mode.
        if (trace.m_pEnt)
        {
            return;
        }
    }

    if (ClientHelpers::SetViewEntity(client, edict))
    {
        if (CBaseEntity* prevRocketEnt = EntityHelpers::HandleToEnt(state.rocket, mServerTools))
        {
            BaseEntityHelpers::SetLocalAngularVelocity(prevRocketEnt, QAngle(0.0f, 0.0f, 0.0f));
        }

        const Vector& localVelocity = BaseEntityHelpers::GetLocalVelocity(rocketEnt);

        state.rocket = rocketHandle;
        state.owner = ownerEntHandle;
        state.initialSpeed = VectorLength(localVelocity);
        state.rollAngle = 0.0f;

        CBaseEntity* ownerEnt = mServerTools->GetBaseEntityByEntIndex(ownerEntIndex);
        assert(ownerEnt);

        BaseEntityHelpers::AddFlag(ownerEnt, FL_FROZEN, mServerGameEnts, mVEngineServer);

        RecipientFilter filter;
        filter.AddAllPlayers(mServer);
        mEngineSound->EmitSound(filter, edict->m_EdictIndex, CHAN_WEAPON, BOOSTER_LOOP, 1.0f, SNDLVL_80dB);

        // currently broken. m_bCritical isn't set yet.
        if (TFProjectileRocketHelpers::IsCritical(rocketEnt))
        {
            mEngineSound->EmitSound(filter, state.rocket.GetEntryIndex(), CHAN_STATIC, CRIT_LOOP, 1.0f, SNDLVL_180dB);
        }
    }
}

void RocketMode::DetachFromRocket(CBaseEntity* rocketEnt)
{
    // if no owner
    CBaseHandle owner = BaseEntityHelpers::GetOwnerEntity(rocketEnt);
    if (!owner.IsValid())
    {
        return;
    }

    // if owner is not a player
    const int ownerEntIndex = owner.GetEntryIndex();
    if (ownerEntIndex <= 0 || ownerEntIndex >= MAX_PLAYERS)
    {
        return;
    }

    const int ownerClientIndex = ownerEntIndex - 1;
    State& state = mClientStates[ownerClientIndex];

    // if we're not tracking this rocket
    if (state.rocket != rocketEnt->GetRefEHandle())
    {
        // Rocket came from a bot.
        // Soldier fired multiple rockets.
        // Client disconnected (ClientDisconnect nulls it).
        return;
    }

    // if the owner changed. Should be handled by SetOwnerEntityHook.
    // This check is for safety.
    if (state.owner != owner)
    {
        return;
    }

    BaseEntityHelpers::SetLocalAngularVelocity(rocketEnt, QAngle(0.0f, 0.0f, 0.0f));

    // Clear state, reset view
    state.Reset();

    IClient* client = mServer->GetClient(ownerClientIndex);
    if (client && ClientHelpers::SetViewEntity(client, nullptr))
    {
        CBaseEntity* ownerEnt = mServerTools->GetBaseEntityByEntIndex(ownerEntIndex);
        assert(ownerEnt);

        BaseEntityHelpers::RemoveFlag(ownerEnt, FL_FROZEN, mServerGameEnts, mVEngineServer);
    }
}

bool RocketMode::ModifyRocketAngularPrecision()
{
    assert(mCvar);
    assert(mServerGameDll);

    mSendTables = mCvar->FindVar("sv_sendtables");
    if (!mSendTables)
    {
        // ensure dev commands are unhidden.
        return false;
    }

    mTFBaseRocketClass = EntityHelpers::GetServerClass(mServerGameDll, "CTFBaseRocket");
    if (!mTFBaseRocketClass)
    {
        return false;
    }

    SendProp* angRotationProp = EntityHelpers::GetProp(mTFBaseRocketClass, "DT_TFBaseRocket", "m_angRotation");
    if (!angRotationProp)
    {
        return false;
    }

    // Increase angular precision:
    // angular precision = (high-low) / ((1 << nBits) - 1)
    // nBits | angular precision (degrees)
    //   6   |   5.7    (default in DT_TFBaseRocket)
    //   7   |   2.8
    //   8   |   1.4
    //   9   |   0.7
    //  10   |   0.35
    //  11   |   0.18
    //  12   |   0.09
    //  13   |   0.04   (default in DT_BaseEntity)

    constexpr int newAngularPrecisionBits = 13;
    angRotationProp->m_nBits = newAngularPrecisionBits;
    double range = static_cast<double>(angRotationProp->m_fHighValue) - angRotationProp->m_fLowValue;
    unsigned long iHighValue = ((1 << (unsigned long)angRotationProp->m_nBits) - 1);

    angRotationProp->m_fHighLowMul = static_cast<float>(iHighValue / range);
    return true;
}

bool RocketMode::PlayerRunCommandHook(CUserCmd* ucmd, IMoveHelper* moveHelper)
{
    RocketMode* thisPtr = sPlayerRunCommandHook.GetThisPtr();
    CBaseEntity* playerEnt = reinterpret_cast<CBaseEntity*>(this);
    thisPtr->PlayerRunCommand(playerEnt, ucmd, moveHelper);
    return sPlayerRunCommandHook.CallOriginalFn(this, ucmd, moveHelper);
}

void RocketMode::PlayerRunCommand(CBaseEntity* player, CUserCmd* ucmd, IMoveHelper* moveHelper)
{
    // log spam fix.
    // DataTable warning: player: Out-of-range value (359.000000/90.000000) in SendPropFloat 'm_angEyeAngles[0]', clamping.
    if (ucmd->viewangles.x > 90.0f)
    {
        ucmd->viewangles.x = 90.0f;
    }

    const CBaseHandle& handle = player->GetRefEHandle();
    const int entIndex = handle.GetEntryIndex();
    const int clientIndex = entIndex - 1;

    State& state = mClientStates[clientIndex];
    if (!state.rocket.IsValid())
    {
        return;
    }

    CBaseEntity* rocketEnt = EntityHelpers::HandleToEnt(state.rocket, mServerTools);
    if (!rocketEnt)
    {
        return;
    }

    const bool attack2 = (ucmd->buttons & IN_ATTACK2);
    if (attack2)
    {
        DetachFromRocket(rocketEnt);
        return;
    }

    // Disable weapnon switching while in rocket mode.
    // Clients will predict incorrectly and flicker the hud a bit.
    // Can't do anything about that.
    ucmd->weaponselect = 0;
    ucmd->weaponsubtype = 0;

    const bool left = (ucmd->buttons & IN_MOVELEFT) != 0;
    const bool right = (ucmd->buttons & IN_MOVERIGHT) != 0;
    const bool up = (ucmd->buttons & IN_FORWARD) != 0;
    const bool down = (ucmd->buttons & IN_BACK) != 0;

    constexpr float turnspeed = 125.0f;

    QAngle localAngVel;
    localAngVel.Init();
    if (left != right)
    {
        localAngVel.y = left ? turnspeed : -turnspeed;
    }
    if (up != down)
    {
        localAngVel.x = up ? -turnspeed : turnspeed;
    }
    BaseEntityHelpers::SetLocalAngularVelocity(rocketEnt, localAngVel);

    //Debug::Msg("frame angVel %i: %f %f %f\n", mGlobals->framecount, localAngVel.x, localAngVel.y, localAngVel.z);

    // m_vecAngVelocity is local angular velocity
    // m_angRotation is local rotation
    // m_vecVelocity is local velocity
    // 
    // m_vecAbsVelocity is world space velocity
    // m_vecBaseVelocity is another world space velocity
    // m_angAbsRotation is world rotation (unused here)

    // Simulation from CBaseEntity::PhysicsToss (simplified, no parent):
    // m_vecAbsVelocity = m_vecVelocity;
    // m_vecAbsOrigin += (m_vecAbsVelocity + m_vecBaseVelocity) * gpGlobals->frametime;
    // m_angRotation += m_vecAngVelocity * gpGlobals->frametime;

    // Update the local velocity assuming our angular velocity to avoid a frame of delay.

    // calculate new angRotation, but don't set it. PhysicsToss will do the same calculation as here.
    QAngle angRotation = BaseEntityHelpers::GetLocalRotation(rocketEnt);
    angRotation += (localAngVel * mGlobals->frametime);

    Vector newVelocity;
    AngleVectors(angRotation, newVelocity);
    newVelocity *= state.initialSpeed;

    BaseEntityHelpers::SetLocalVelocity(rocketEnt, newVelocity);
}

void RocketMode::SetOwnerEntityHook(CBaseEntity* owner)
{
    RocketMode* thisPtr = sPlayerRunCommandHook.GetThisPtr();
    CBaseEntity* rocket = reinterpret_cast<CBaseEntity*>(this);
    thisPtr->SetOwnerEntity(rocket, owner);
    sSetOwnerEntityHook.CallOriginalFn(this, owner);
}

void RocketMode::SetOwnerEntity(CBaseEntity* rocket, CBaseEntity* newOwner)
{
    CBaseHandle ownerHandle = BaseEntityHelpers::GetOwnerEntity(rocket);
    if (ownerHandle.IsValid() && (ownerHandle != newOwner->GetRefEHandle()))
    {
        DetachFromRocket(rocket);
    }
}

void RocketMode::RocketSpawn(CBaseEntity* rocket)
{
    CBaseHandle launcherHandle = TFBaseRocketHelpers::GetLauncher(rocket);
    if (!launcherHandle.IsValid())
    {
        return;
    }

    CBaseEntity* launcher = EntityHelpers::HandleToEnt(launcherHandle, mServerTools);
    if (!launcher)
    {
        return;
    }

    IHasAttributes* attributeInterface = BaseEntityHelpers::GetAttribInterface(launcher);
    assert(attributeInterface);
    CAttributeContainer* con = attributeInterface->GetAttributeContainer();
    assert(con);

    CEconItemView& item = AttributeContainerHelpers::GetItem(con);

    if (item.m_iItemID == 3977757014)
    {
        AttachToRocket(rocket);
    }
}

void RocketMode::RocketChangeTeamHook(int team)
{
    RocketMode* thisPtr = sRocketChangeTeamHook.GetThisPtr();
    CBaseEntity* rocket = reinterpret_cast<CBaseEntity*>(this);
    const int oldTeam = BaseEntityHelpers::GetTeam(rocket);
    sRocketChangeTeamHook.CallOriginalFn(this, team);
    thisPtr->RocketChangeTeam(rocket, oldTeam);
}

void RocketMode::RocketChangeTeam(CBaseEntity* rocket, int oldTeam)
{
    // Called right after spawn. Can treat as initialization
    if (oldTeam == 0)
    {
        RocketSpawn(rocket);
        return;
    }

    // Called from somewhere else, probably pyro deflect.
    // SetOwnerEntityHook will handle that.
}

static IClient* UserIdToClient(int userid, IServer* server)
{
    const int clientCount = server->GetClientCount();
    for (int i = 0; i < clientCount; ++i)
    {
        IClient* client = server->GetClient(i);
        if (client && client->IsActive())
        {
            if (client->GetUserID() == userid)
            {
                return client;
            }
        }
    }
    return nullptr;
}

DEFINE_INHERITED_DESTRUCTOR(RocketMode, IGameEventListener2);

void RocketMode::FireGameEvent(IGameEvent* event)
{
    const char* eventName = event->GetName();
    if (!strcmp(eventName, "player_spawn"))
    {
        const int userid = event->GetInt("userid", -1);
        IClient* client = UserIdToClient(userid, mServer);
        if (client)
        {
            edict_t* viewEdict = ClientHelpers::GetViewEntity(client);
            if (viewEdict)
            {
                ClientHelpers::SetViewEntity(client, nullptr);
            }
        }
    }
    else if (!strcmp(eventName, "player_death"))
    {
        // from CTFGameRules::DeathNotice
        const int victimEntIndex = event->GetInt("victim_entindex", -1);
        if (victimEntIndex > 0 && victimEntIndex < MAX_PLAYERS)
        {
            const int victimClientIndex = victimEntIndex - 1;
            State& state = mClientStates[victimClientIndex];
            if (state.rocket.IsValid())
            {
                CBaseEntity* rocket = EntityHelpers::HandleToEnt(state.rocket, mServerTools);
                DetachFromRocket(rocket);
            }
        }
    }
}
