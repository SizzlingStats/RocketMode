
#include "RocketMode.h"

#include "sourcehelpers/ClientHelpers.h"
#include "sourcehelpers/EntityHelpers.h"

#include "sourcesdk/game/server/baseentity.h"
#include "sourcesdk/game/server/iplayerinfo.h"
#include "sourcesdk/game/shared/econ/econ_item_view.h"
#include "sourcesdk/game/shared/econ/ihasattributes.h"
#include "sourcesdk/game/shared/in_buttons.h"
#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/game/shared/teamplayroundbased_gamerules.h"
#include "sourcesdk/game/shared/usercmd.h"
#include "sourcesdk/public/engine/IEngineSound.h"
#include "sourcesdk/public/basehandle.h"
#include "sourcesdk/public/tier1/bitbuf.h"
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

#include "sourcehelpers/NetMessageHelpers.h"
#include "sourcehelpers/RecipientFilter.h"
#include "sourcehelpers/SendTablesFix.h"
#include "sourcehelpers/Vector.h"
#include "base/math.h"
#include "base/stringbuilder.h"
#include "HookOffsets.h"
#include "SizzLauncherInfo.h"
#include <string.h>

#define BOOSTER_LOOP ")ambient/steam_drum.wav"
#define CRIT_LOOP "weapons/crit_power.wav"

string_t RocketMode::tf_projectile_rocket;
VTableHook<decltype(&RocketMode::WeaponEquipHook)> RocketMode::sWeaponEquipHook;
VTableHook<decltype(&RocketMode::GetNextObserverSearchStartPointHook)> RocketMode::sGetNextObserverSearchStartPointHook;
VTableHook<decltype(&RocketMode::PlayerRunCommandHook)> RocketMode::sPlayerRunCommandHook;
VTableHook<decltype(&RocketMode::SetOwnerEntityHook)> RocketMode::sSetOwnerEntityHook;
VTableHook<decltype(&RocketMode::RocketChangeTeamHook)> RocketMode::sRocketChangeTeamHook;
VTableHook<decltype(&RocketMode::RocketIsDeflectableHook)> RocketMode::sIsDeflectableHook;
VTableHook<decltype(&RocketMode::FuncRespawnRoomStartTouchHook)> RocketMode::sFuncRespawnRoomStartTouchHook;

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
    mClientStates(),
    mSpecStates()
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

    if (!mGameEventManager->AddListener(this, "player_death", true))
    {
        return false;
    }

    if (!ModifyRocketAngularPrecision())
    {
        return false;
    }

    TeamplayRoundBasedRulesHelpers::InitializeOffsets(mServerGameDll);

    return true;
}

void RocketMode::Shutdown()
{
    if (mGameEventManager)
    {
        mGameEventManager->RemoveListener(this);
    }
    sFuncRespawnRoomStartTouchHook.Unhook();
    sIsDeflectableHook.Unhook();
    sGetNextObserverSearchStartPointHook.Unhook();
    sWeaponEquipHook.Unhook();
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
        if (!sIsDeflectableHook.GetThisPtr())
        {
            sIsDeflectableHook.Hook(ent, HookOffsets::IsDeflectable, this, &RocketMode::RocketIsDeflectableHook);
        }
        mServerTools->RemoveEntityImmediate(ent);
    }
}

void RocketMode::ServerActivate(CGameRules* gameRules)
{
    mGameRules = gameRules;

    if (!sFuncRespawnRoomStartTouchHook.GetThisPtr())
    {
        CBaseEntity* funcRespawnRoom = mServerTools->FindEntityByClassname(nullptr, "func_respawnroom");
        if (funcRespawnRoom)
        {
            BaseTriggerHelpers::InitializeOffsets(funcRespawnRoom);
            sFuncRespawnRoomStartTouchHook.Hook(funcRespawnRoom, HookOffsets::StartTouch, this, &RocketMode::FuncRespawnRoomStartTouchHook);
        }
    }
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

    if (tf_projectile_rocket)
    {
        for (int i = 0; i < clientCount; ++i)
        {
            IClient* client = mServer->GetClient(i);
            if (!client || !client->IsActive() || client->IsFakeClient())
            {
                continue;
            }

            const int entIndex = i + 1;
            CBaseEntity* player = mServerTools->GetBaseEntityByEntIndex(entIndex);
            if (!player)
            {
                continue;
            }

            SpecState& state = mSpecStates[i];

            const int observerMode = BasePlayerHelpers::GetObserverMode(player);
            if (observerMode == OBS_MODE_FIXED || observerMode == OBS_MODE_IN_EYE || observerMode == OBS_MODE_CHASE)
            {
                const CBaseHandle targetHandle = BasePlayerHelpers::GetObserverTarget(player);
                CBaseEntity* targetEnt = EntityHelpers::HandleToEnt(targetHandle, mServerTools);
                if (targetEnt)
                {
                    if (tf_projectile_rocket == BaseEntityHelpers::GetClassname(targetEnt))
                    {
                        const CBaseHandle rocketOwnerHandle = BaseEntityHelpers::GetOwnerEntity(targetEnt);
                        const CBaseEntity* rocketOwner = EntityHelpers::HandleToEnt(rocketOwnerHandle, mServerTools);
                        if (rocketOwner && (rocketOwner != player))
                        {
                            // client is spectating a rocket that's not its own.
                            if ((state.rocketSpecTarget != targetHandle) || (state.nextHintSendTick <= mGlobals->tickcount))
                            {
                                state.nextHintSendTick = mGlobals->tickcount + static_cast<int>(5.0f / mGlobals->interval_per_tick);
                                state.rocketSpecTarget = targetHandle;

                                const char* ownerName = BasePlayerHelpers::GetPlayerName(rocketOwner);
                                StringBuilder<64> targetId;
                                targetId.Append(ownerName);
                                const char lastChar = targetId.last();
                                targetId.Append('\'');
                                if (lastChar != 's' && lastChar != 'S')
                                {
                                    targetId.Append('s');
                                }
                                targetId.Append(" Rocket");

                                RecipientFilter filter;
                                filter.SetSingleRecipient(entIndex);
                                bf_write* buf = mVEngineServer->UserMessageBegin(&filter, 19); // HintText
                                buf->WriteString(targetId.c_str());
                                mVEngineServer->MessageEnd();
                            }
                            continue;
                        }
                    }
                }
            }

            if (state.rocketSpecTarget.IsValid())
            {
                state.nextHintSendTick = 0;
                state.rocketSpecTarget.Term();

                RecipientFilter filter;
                filter.SetSingleRecipient(entIndex);
                bf_write* buf = mVEngineServer->UserMessageBegin(&filter, 6); // ResetHUD
                buf->WriteByte(0);
                mVEngineServer->MessageEnd();

            }
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

bool RocketMode::ClientCommand(edict_t* pEntity, const CCommand& args)
{
    const char* command = args.Arg(0);

    const bool bSpecPrev = !strcmp(command, "spec_prev");
    if (bSpecPrev ||
        !strcmp(command, "spec_next") ||
        !strcmp(command, "spec_mode") ||
        !strcmp(command, "spec_menu") ||
        !strcmp(command, "spec_autodirector"))
    {
        const int clientIndex = pEntity->m_EdictIndex - 1;
        if (clientIndex >= 0 && clientIndex < MAX_PLAYERS)
        {
            State& state = mClientStates[clientIndex];
            if (state.rocket.IsValid())
            {
                if (bSpecPrev)
                {
                    CBaseEntity* rocketEnt = EntityHelpers::HandleToEnt(state.rocket, mServerTools);
                    if (rocketEnt)
                    {
                        DetachFromRocket(rocketEnt);
                    }
                }
                return true;
            }
        }
    }
    return false;
}

void RocketMode::ClientActive(edict_t* pEntity)
{
    if (!sWeaponEquipHook.GetThisPtr())
    {
        CBaseEntity* ent = mServerTools->GetBaseEntityByEntIndex(pEntity->m_EdictIndex);
        assert(ent);

        sWeaponEquipHook.Hook(ent, HookOffsets::Weapon_Equip, this, &RocketMode::WeaponEquipHook);
    }
    if (!sGetNextObserverSearchStartPointHook.GetThisPtr())
    {
        CBaseEntity* ent = mServerTools->GetBaseEntityByEntIndex(pEntity->m_EdictIndex);
        assert(ent);

        sGetNextObserverSearchStartPointHook.Hook(ent, HookOffsets::GetNextObserverSearchStartPoint, this, &RocketMode::GetNextObserverSearchStartPointHook);
    }
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
        mEngineSound->StopSound(rocketEdict->m_EdictIndex, CHAN_STATIC, BOOSTER_LOOP);
        mEngineSound->StopSound(rocketEdict->m_EdictIndex, CHAN_BODY, CRIT_LOOP);
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
    if (ownerEntIndex <= 0 || ownerEntIndex >= mGlobals->maxClients)
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

    CBaseEntity* ownerEnt = mServerTools->GetBaseEntityByEntIndex(ownerEntIndex);
    assert(ownerEnt);

    BasePlayerHelpers::SetObserverMode(ownerEnt, OBS_MODE_FIXED, mServerGameEnts, mVEngineServer);
    BasePlayerHelpers::SetObserverLastMode(ownerEnt, OBS_MODE_FIXED, mServerGameEnts, mVEngineServer);
    BasePlayerHelpers::SetObserverTarget(ownerEnt, rocketHandle, mServerGameEnts, mVEngineServer);
    BasePlayerHelpers::SetForcedObserverMode(ownerEnt, false, mServerGameEnts, mVEngineServer);

    // Bandaid for client prediction causing weird view offset errors.
    // To reproduce, stand on a slanted surface and shoot rocket.
    SetSingleConvar setsingleconvar;
    setsingleconvar.Set("sv_client_predict", "0");
    client->SendNetMsg(setsingleconvar, true);

    ClientHelpers::SetViewEntity(client, edict);

    // send all spectators of this player or the player's current rocket to the new rocket
    {
        const int clientCount = mServer->GetClientCount();
        for (int i = 0; i < clientCount; ++i)
        {
            IClient* client = mServer->GetClient(i);
            if (!client->IsActive() || client->IsFakeClient())
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

            // if spectating a player
            const int observerMode = BasePlayerHelpers::GetObserverMode(clientEnt);
            if (observerMode == OBS_MODE_IN_EYE || observerMode == OBS_MODE_CHASE)
            {
                // OBS_MODE_CHASE seems to be set when on a fixed map view.
                // I thought it would be OBS_MODE_FIXED, but whatever.
                CBaseHandle observerTarget = BasePlayerHelpers::GetObserverTarget(clientEnt);
                const bool spectatingPrevRocket = state.rocket.IsValid() && (observerTarget == state.rocket);
                const bool spectatingOwner = observerTarget == ownerEntHandle;
                if (spectatingPrevRocket || spectatingOwner)
                {
                    BasePlayerHelpers::SetObserverTarget(clientEnt, rocketHandle, mServerGameEnts, mVEngineServer);
                }
            }
        }
    }

    if (CBaseEntity* prevRocketEnt = EntityHelpers::HandleToEnt(state.rocket, mServerTools))
    {
        BaseEntityHelpers::SetLocalAngularVelocity(prevRocketEnt, QAngle(0.0f, 0.0f, 0.0f));
    }

    state.rocket = rocketHandle;
    state.owner = ownerEntHandle;
    state.initialSpeed = 0.0f;
    state.rollAngle = 0.0f;

    BaseEntityHelpers::AddFlag(ownerEnt, FL_FROZEN, mServerGameEnts, mVEngineServer);

    RecipientFilter filter;
    filter.AddAllPlayers(mServer);
    mEngineSound->EmitSound(filter, edict->m_EdictIndex, CHAN_STATIC, BOOSTER_LOOP, 1.0f, SNDLVL_NORM);
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
    if (ownerEntIndex <= 0 || ownerEntIndex >= mGlobals->maxClients)
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

    // send all spectators of this rocket back to the player that fired it.
    {
        const int clientCount = mServer->GetClientCount();
        for (int i = 0; i < clientCount; ++i)
        {
            IClient* client = mServer->GetClient(i);
            if (!client->IsActive() || client->IsFakeClient())
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

            // if spectating a player
            const int observerMode = BasePlayerHelpers::GetObserverMode(clientEnt);
            if (observerMode == OBS_MODE_IN_EYE || observerMode == OBS_MODE_CHASE)
            {
                // OBS_MODE_CHASE seems to be set when on a fixed map view.
                // I thought it would be OBS_MODE_FIXED, but whatever.
                CBaseHandle observerTarget = BasePlayerHelpers::GetObserverTarget(clientEnt);
                if (observerTarget.IsValid() && (observerTarget == state.rocket))
                {
                    // spectate the owner again
                    BasePlayerHelpers::SetObserverTarget(clientEnt, owner, mServerGameEnts, mVEngineServer);
                }
            }
        }
    }

    BaseEntityHelpers::SetLocalAngularVelocity(rocketEnt, QAngle(0.0f, 0.0f, 0.0f));

    CBaseEntity* ownerEnt = mServerTools->GetBaseEntityByEntIndex(ownerEntIndex);
    assert(ownerEnt);

    BasePlayerHelpers::SetObserverMode(ownerEnt, OBS_MODE_NONE, mServerGameEnts, mVEngineServer);
    BasePlayerHelpers::SetObserverLastMode(ownerEnt, OBS_MODE_NONE, mServerGameEnts, mVEngineServer);
    BasePlayerHelpers::SetObserverTarget(ownerEnt, CBaseHandle(), mServerGameEnts, mVEngineServer);
    BasePlayerHelpers::SetForcedObserverMode(ownerEnt, false, mServerGameEnts, mVEngineServer);

    // Clear state, reset view
    state.Reset();

    IClient* client = mServer->GetClient(ownerClientIndex);
    assert(client);

    ClientHelpers::SetViewEntity(client, nullptr);

    SetSingleConvar setsingleconvar;
    setsingleconvar.Set("sv_client_predict", "1");
    client->SendNetMsg(setsingleconvar, true);

    BaseEntityHelpers::RemoveFlag(ownerEnt, FL_FROZEN, mServerGameEnts, mVEngineServer);
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

void RocketMode::WeaponEquipHook(CBaseEntity* weapon)
{
    RocketMode* thisPtr = sWeaponEquipHook.GetThisPtr();
    CBaseEntity* playerEnt = reinterpret_cast<CBaseEntity*>(this);
    thisPtr->WeaponEquip(playerEnt, weapon);
    sWeaponEquipHook.CallOriginalFn(this, weapon);
}

void RocketMode::WeaponEquip(CBaseEntity* player, CBaseEntity* weapon)
{
    const CBaseHandle& handle = player->GetRefEHandle();
    const int entIndex = handle.GetEntryIndex();
    const int clientIndex = entIndex - 1;

    State& state = mClientStates[clientIndex];
    if (state.rocket.IsValid())
    {
        CBaseEntity* rocketEnt = EntityHelpers::HandleToEnt(state.rocket, mServerTools);
        if (rocketEnt)
        {
            DetachFromRocket(rocketEnt);
        }
    }
}

int RocketMode::GetNextObserverSearchStartPointHook(bool bReverse)
{
    // This returns the next spec target after the current target (depending on bReverse).
    // We'll make it up ourselves.
    sGetNextObserverSearchStartPointHook.CallOriginalFn(this, bReverse);
    RocketMode* thisPtr = sGetNextObserverSearchStartPointHook.GetThisPtr();
    CBaseEntity* playerEnt = reinterpret_cast<CBaseEntity*>(this);
    return thisPtr->GetNextObserverSearchStartPoint(playerEnt, bReverse);
}

int RocketMode::GetNextObserverSearchStartPoint(CBaseEntity* player, bool bReverse)
{
    const CBaseHandle currentObserverTarget = BasePlayerHelpers::GetObserverTarget(player);
    CUtlVector<CBaseHandle>& observableEntities = TFPlayerHelpers::GetObservableEntities(player);
    const int maxClients = mGlobals->maxClients;
    int newObserverIndex = -1;

    // Add any rocket mode rockets to the observable entities list.
    // A player's rocket is inserted right after the player.
    for (int i = 0; i < observableEntities.Count(); ++i)
    {
        CBaseHandle ent = observableEntities.Element(i);
        if (currentObserverTarget == ent)
        {
            newObserverIndex = i;
        }

        const int entIndex = ent.GetEntryIndex();
        if (entIndex > 0 && entIndex <= maxClients)
        {
            const int clientIndex = entIndex - 1;
            assert((clientIndex >= 0) && (clientIndex < MAX_PLAYERS));

            const State& state = mClientStates[clientIndex];
            if (state.rocket.IsValid())
            {
                i = observableEntities.InsertAfter(i, state.rocket);
                if (currentObserverTarget == state.rocket)
                {
                    newObserverIndex = i;
                }
            }
        }
    }

    // handle wraparound
    const int numObservableEntities = observableEntities.Count();
    newObserverIndex += (bReverse ? -1 : 1);
    if (newObserverIndex >= numObservableEntities)
    {
        newObserverIndex = 0;
    }
    else if (newObserverIndex < 0)
    {
        newObserverIndex = numObservableEntities - 1;
    }
    return newObserverIndex;
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

    // Now handled by ClientCommand spec_prev
    //const bool attack2 = (ucmd->buttons & IN_ATTACK2);
    //if (attack2)
    //{
    //    DetachFromRocket(rocketEnt);
    //    return;
    //}

    if (state.initialSpeed == 0.0f)
    {
        const Vector& localVelocity = BaseEntityHelpers::GetLocalVelocity(rocketEnt);
        state.initialSpeed = VectorLength(localVelocity);

        if (TFProjectileRocketHelpers::IsCritical(rocketEnt))
        {
            RecipientFilter filter;
            filter.AddAllPlayers(mServer);
            mEngineSound->EmitSound(filter, state.rocket.GetEntryIndex(), CHAN_BODY, CRIT_LOOP, 1.0f, SNDLVL_NORM);
        }
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
    RocketMode* thisPtr = sSetOwnerEntityHook.GetThisPtr();
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
    CBaseEntity* weapon = EntityHelpers::HandleToEnt(launcherHandle, mServerTools);
    if (!weapon)
    {
        // If the owner switches loadout while a rocket is in the air.
        return;
    }

    CEconItemView* item = EntityHelpers::GetEconItemFromWeapon(weapon, mServerTools);
    if (!item)
    {
        return;
    }

    if (item->m_iItemID == SizzLauncherInfo::ItemID)
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

bool RocketMode::RocketIsDeflectableHook()
{
    RocketMode* thisPtr = sIsDeflectableHook.GetThisPtr();
    CBaseEntity* rocketEnt = reinterpret_cast<CBaseEntity*>(this);
    if (thisPtr->RocketIsDeflectable(rocketEnt))
    {
        return sIsDeflectableHook.CallOriginalFn(this);
    }
    return false;
}

bool RocketMode::RocketIsDeflectable(CBaseEntity* rocketEnt)
{
    CBaseHandle launcherHandle = TFBaseRocketHelpers::GetLauncher(rocketEnt);
    CBaseEntity* weapon = EntityHelpers::HandleToEnt(launcherHandle, mServerTools);
    if (!weapon)
    {
        // This can happen if the owner switches loadout while a rocket is in the air.
        // The rocket will be deflectable in this case because we don't know if it was
        // a rocket mode rocket or not.
        return true;
    }

    CEconItemView* item = EntityHelpers::GetEconItemFromWeapon(weapon, mServerTools);
    if (!item)
    {
        return true;
    }

    // rocket mode rockets are not deflectable
    return (item->m_iItemID != SizzLauncherInfo::ItemID);
}

void RocketMode::FuncRespawnRoomStartTouchHook(CBaseEntity* other)
{
    RocketMode* thisPtr = sFuncRespawnRoomStartTouchHook.GetThisPtr();
    CBaseEntity* respawnRoom = reinterpret_cast<CBaseEntity*>(this);
    thisPtr->FuncRespawnRoomStartTouch(respawnRoom, other);
    sFuncRespawnRoomStartTouchHook.CallOriginalFn(this, other);
}

void RocketMode::FuncRespawnRoomStartTouch(CBaseEntity* respawnRoom, CBaseEntity* other)
{
    assert(tf_projectile_rocket);
    const string_t classname = BaseEntityHelpers::GetClassname(other);
    if (classname != tf_projectile_rocket)
    {
        return;
    }

    // should probably make this type safe by defining the whole CTeamplayRoundBasedRules hierarchy.
    CTeamplayRoundBasedRules* rules = reinterpret_cast<CTeamplayRoundBasedRules*>(mGameRules);
    const int roundState = TeamplayRoundBasedRulesHelpers::GetRoundState(rules);
    if (roundState >= GR_STATE_TEAM_WIN)
    {
        return;
    }

    if (BaseTriggerHelpers::IsDisabled(respawnRoom))
    {
        return;
    }

    const int respawnRoomTeam = BaseEntityHelpers::GetTeam(respawnRoom);
    const int rocketTeam = BaseEntityHelpers::GetTeam(other);
    if (respawnRoomTeam != rocketTeam)
    {
        DetachFromRocket(other);
    }
}

DEFINE_INHERITED_DESTRUCTOR(RocketMode, IGameEventListener2);

void RocketMode::FireGameEvent(IGameEvent* event)
{
    const char* eventName = event->GetName();
    assert(!strcmp(eventName, "player_death"));

    // sent from CTFGameRules::DeathNotice
    const int victimEntIndex = event->GetInt("victim_entindex", -1);
    const int victimClientIndex = victimEntIndex - 1;
    if (victimClientIndex >= 0 && victimClientIndex < MAX_PLAYERS)
    {
        State& state = mClientStates[victimClientIndex];
        if (state.rocket.IsValid())
        {
            CBaseEntity* rocket = EntityHelpers::HandleToEnt(state.rocket, mServerTools);
            DetachFromRocket(rocket);
        }
    }
}
