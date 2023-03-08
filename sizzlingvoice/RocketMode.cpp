
#include "RocketMode.h"

#include "sourcehelpers/DatamapHelpers.h"
#include "sourcehelpers/EdictChangeHelpers.h"
#include "sourcehelpers/NetPropHelpers.h"

#include "sourcesdk/common/netmessages.h"
#include "sourcesdk/game/server/baseentity.h"
#include "sourcesdk/game/server/iplayerinfo.h"
#include "sourcesdk/game/shared/in_buttons.h"
#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/game/shared/usercmd.h"
#include "sourcesdk/public/basehandle.h"
#include "sourcesdk/public/const.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/globalvars_base.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/icvar.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/iservernetworkable.h"
#include "sourcesdk/public/iserverunknown.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/public/toolframework/itoolentity.h"

#include "base/math.h"

string_t RocketMode::tf_projectile_rocket;
int RocketMode::sClassnameOffset;
int RocketMode::sOwnerEntityOffset;
int RocketMode::sfFlagsOffset;
int RocketMode::seFlagsOffset;
int RocketMode::sLocalVelocityOffset;
int RocketMode::sAngRotationOffset;
int RocketMode::sAngVelocityOffset;
VTableHook<decltype(&RocketMode::PlayerRunCommandHook)> RocketMode::sPlayerRunCommandHook;

string_t RocketMode::GetClassname(CBaseEntity* ent)
{
    assert(sClassnameOffset > 0);
    return *(string_t*)((char*)ent + sClassnameOffset);
}

CBaseHandle RocketMode::GetOwnerEntity(CBaseEntity* ent)
{
    assert(sOwnerEntityOffset > 0);
    return *(CBaseHandle*)((char*)ent + sOwnerEntityOffset);
}

RocketMode::RocketMode() :
    mVEngineServer(nullptr),
    mServer(nullptr),
    mServerTools(nullptr),
    mServerGameDll(nullptr),
    mCvar(nullptr),
    mGlobals(nullptr),
    mClientStates()
{
}

RocketMode::~RocketMode()
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

    IPlayerInfoManager* playerInfoManager = (IPlayerInfoManager*)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, nullptr);
    if (playerInfoManager)
    {
        mGlobals = reinterpret_cast<CGlobalVarsBase*>(playerInfoManager->GetGlobalVars());
    }

    if (!mVEngineServer || !mServer || !mServerTools || !mCvar || !mGlobals)
    {
        return false;
    }

    //SendProp* angRotationProp = NetPropHelpers::GetProp(mServerGameDll, "CTFProjectile_Rocket", "DT_TFBaseRocket", "m_angRotation");
    //if (angRotationProp)
    //{
    //    angRotationProp->m_nBits = 13;
    //    double range = static_cast<double>(angRotationProp->m_fHighValue) - angRotationProp->m_fLowValue;
    //    unsigned long iHighValue = ((1 << (unsigned long)angRotationProp->m_nBits) - 1);

    //    angRotationProp->m_fHighLowMul = static_cast<float>(iHighValue / range);
    //}
    //mCvar->FindVar("sv_sendtables")->SetValue(1);

    return true;
}

void RocketMode::Shutdown()
{
    sPlayerRunCommandHook.Unhook();
}

void RocketMode::LevelInit(const char* pMapName)
{
    CBaseEntity* ent = mServerTools->CreateEntityByName("tf_projectile_rocket");
    if (ent)
    {
        datamap_t* datamap = ent->GetDataDescMap();
        assert(datamap);

        if (!sClassnameOffset)
        {
            sClassnameOffset = DatamapHelpers::GetDatamapVarOffset(datamap, "m_iClassname");
            assert(sClassnameOffset > 0);
        }
        if (!sOwnerEntityOffset)
        {
            sOwnerEntityOffset = DatamapHelpers::GetDatamapVarOffset(datamap, "m_hOwnerEntity");
            assert(sOwnerEntityOffset > 0);
        }
        if (!sfFlagsOffset)
        {
            sfFlagsOffset = DatamapHelpers::GetDatamapVarOffset(datamap, "m_fFlags");
            assert(sfFlagsOffset > 0);
        }
        if (!seFlagsOffset)
        {
            seFlagsOffset = DatamapHelpers::GetDatamapVarOffset(datamap, "m_iEFlags");
            assert(seFlagsOffset > 0);
        }
        if (!sLocalVelocityOffset)
        {
            sLocalVelocityOffset = DatamapHelpers::GetDatamapVarOffset(datamap, "m_vecVelocity");
            assert(sLocalVelocityOffset > 0);
        }
        if (!sAngRotationOffset)
        {
            sAngRotationOffset = DatamapHelpers::GetDatamapVarOffset(datamap, "m_angRotation");
            assert(sAngRotationOffset > 0);
        }
        if (!sAngVelocityOffset)
        {
            sAngVelocityOffset = DatamapHelpers::GetDatamapVarOffset(datamap, "m_vecAngVelocity");
            assert(sAngVelocityOffset > 0);
        }

        tf_projectile_rocket = *(string_t*)((char*)ent + sClassnameOffset);
        mServerTools->RemoveEntityImmediate(ent);
    }
}

void RocketMode::LevelShutdown()
{
    tf_projectile_rocket = nullptr;
}

void RocketMode::ClientActive(edict_t* pEntity)
{
    if (!sPlayerRunCommandHook.GetThisPtr())
    {
        CBaseEntity* ent = mServerTools->GetBaseEntityByEntIndex(pEntity->m_EdictIndex);
        assert(ent);

#ifdef SDK_COMPAT
        constexpr int Offset = 421;
#else
        constexpr int Offset = 430;
#endif
        sPlayerRunCommandHook.Hook(ent, Offset, this, &RocketMode::PlayerRunCommandHook);
    }
}

void RocketMode::ClientDisconnect(edict_t* pEntity)
{
    // When clients disconnect, their projectiles are instantly destroyed.
    // OnEntityDeleted will see a projectile with an owner edict marked FL_KILLME.

    // Clear out rocket and owner here just to make things less error prone.
    const int clientIndex = pEntity->m_EdictIndex - 1;
    assert(clientIndex < MAX_PLAYERS);
    State& state = mClientStates[clientIndex];
    state.rocket = nullptr;
    state.owner.Term();
}

void RocketMode::OnEntitySpawned(CBaseEntity* pEntity)
{
    const string_t classname = GetClassname(pEntity);
    if (!tf_projectile_rocket || !classname || (classname != tf_projectile_rocket))
    {
        return;
    }

    IServerNetworkable* networkable = pEntity->GetNetworkable();
    if (!networkable)
    {
        return;
    }

    edict_t* edict = networkable->GetEdict();
    if (!edict)
    {
        return;
    }

    CBaseHandle ownerEntHandle = GetOwnerEntity(pEntity);
    if (!ownerEntHandle.IsValid())
    {
        return;
    }

    const int ownerEntIndex = ownerEntHandle.GetEntryIndex();
    if (ownerEntIndex <= 0 || ownerEntIndex >= MAX_PLAYERS)
    {
        // sentries? TODO: check
        return;
    }

    const int ownerClientIndex = ownerEntIndex - 1;
    IClient* client = mServer->GetClient(ownerClientIndex);
    if (!client || client->IsFakeClient())
    {
        return;
    }

    SVC_SetView setview;
    setview.m_nEntityIndex = edict->m_EdictIndex;

    if (client->SendNetMsg(setview, true))
    {
        State& state = mClientStates[ownerClientIndex];
        state.rocket = pEntity;
        state.owner = ownerEntHandle;

        edict_t* ownerEdict = mVEngineServer->PEntityOfEntIndex(ownerEntIndex);
        CBaseEntity* ownerEnt = mServerTools->GetBaseEntityByEntIndex(ownerEntIndex);
        assert(ownerEdict);
        assert(ownerEnt);

        assert(sfFlagsOffset > 0);
        *(int*)((char*)ownerEnt + sfFlagsOffset) |= FL_ATCONTROLS;
        EdictChangeHelpers::StateChanged(ownerEdict, sfFlagsOffset, mVEngineServer);
    }
}

void RocketMode::OnEntityDeleted(CBaseEntity* pEntity)
{
    const string_t classname = GetClassname(pEntity);
    if (!tf_projectile_rocket || !classname || (classname != tf_projectile_rocket))
    {
        return;
    }

    CBaseHandle owner = GetOwnerEntity(pEntity);
    if (!owner.IsValid())
    {
        return;
    }

    const int ownerEntIndex = owner.GetEntryIndex();
    if (ownerEntIndex <= 0 || ownerEntIndex >= MAX_PLAYERS)
    {
        // sentries? TODO: check
        return;
    }

    const int ownerClientIndex = ownerEntIndex - 1;
    State& state = mClientStates[ownerClientIndex];

    if (state.rocket != pEntity)
    {
        // Rocket came from a bot.
        // Soldier fired multiple rockets.
        // Client disconnected (ClientDisconnect nulls it).
        return;
    }

    if (state.owner != owner)
    {
        // Pyro deflected soldier rocket.
        return;
    }

    // Clear state, reset view
    state.rocket = nullptr;
    state.owner.Term();

    SVC_SetView setview;
    setview.m_nEntityIndex = ownerEntIndex;

    IClient* client = mServer->GetClient(ownerClientIndex);
    if (client && client->SendNetMsg(setview, true))
    {
        edict_t* ownerEdict = mVEngineServer->PEntityOfEntIndex(ownerEntIndex);
        CBaseEntity* ownerEnt = mServerTools->GetBaseEntityByEntIndex(ownerEntIndex);
        assert(ownerEdict);
        assert(ownerEnt);

        assert(sfFlagsOffset > 0);
        *(int*)((char*)ownerEnt + sfFlagsOffset) &= ~FL_ATCONTROLS;
        EdictChangeHelpers::StateChanged(ownerEdict, sfFlagsOffset, mVEngineServer);
    }
}

bool RocketMode::PlayerRunCommandHook(CUserCmd* ucmd, IMoveHelper* moveHelper)
{
    RocketMode* thisPtr = sPlayerRunCommandHook.GetThisPtr();
    CBaseEntity* playerEnt = reinterpret_cast<CBaseEntity*>(this);
    thisPtr->PlayerRunCommand(playerEnt, ucmd, moveHelper);
    return sPlayerRunCommandHook.CallOriginalFn(this, ucmd, moveHelper);
}

inline float DegToRad(float deg)
{
    return deg * (3.14159f / 180.f);
}

static void AngleVectors(const QAngle& angles, Vector& forward)
{
    const float yaw = DegToRad(angles.y);
    const float pitch = DegToRad(angles.x);

    const float sy = Math::Sin(yaw);
    const float cy = Math::Cos(yaw);
    const float sp = Math::Sin(pitch);
    const float cp = Math::Cos(pitch);

    forward.x = cp * cy;
    forward.y = cp * sy;
    forward.z = -sp;
}

inline float VectorLength(const Vector& v)
{
    return Math::Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
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
    if (!state.rocket)
    {
        return;
    }

    // Disable weapnon switching while in rocket mode.
    // Clients will predict incorrectly and flicker the hud a bit.
    // Can't do anything about that.
    ucmd->weaponselect = 0;

    const bool left = (ucmd->buttons & IN_MOVELEFT) != 0;
    const bool right = (ucmd->buttons & IN_MOVERIGHT) != 0;
    const bool up = (ucmd->buttons & IN_FORWARD) != 0;
    const bool down = (ucmd->buttons & IN_BACK) != 0;

    const float turnspeed = 100.0f;

    QAngle angVel;
    angVel.Init();
    if (left != right)
    {
        angVel.y = left ? turnspeed : -turnspeed;
    }
    if (up != down)
    {
        angVel.x = up ? -turnspeed : turnspeed;
    }

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
    Vector& localVelocity = *(Vector*)((char*)state.rocket + sLocalVelocityOffset);
    QAngle& localAngVelocity = *(QAngle*)((char*)state.rocket + sAngVelocityOffset);
    const float speed = VectorLength(localVelocity);

    // calculate new angRotation, but don't set it. PhysicsToss will do the same calculation as here.
    QAngle angRotation = *(QAngle*)((char*)state.rocket + sAngRotationOffset);
    angRotation += (localAngVelocity * mGlobals->frametime);

    Vector newVelocity;
    AngleVectors(angRotation, newVelocity);
    newVelocity *= speed;

    localVelocity = newVelocity;
    localAngVelocity = angVel;

    int& eflags = *(int*)((char*)state.rocket + seFlagsOffset);
    eflags |= EFL_DIRTY_ABSVELOCITY;
}
