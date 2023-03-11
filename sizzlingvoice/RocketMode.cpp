
#include "RocketMode.h"

#include "sourcehelpers/ClientHelpers.h"
#include "sourcehelpers/EntityHelpers.h"

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
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/icvar.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/mathlib/mathlib.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/public/toolframework/itoolentity.h"

#include "base/math.h"
#include "sourcehelpers/SendTablesFix.h"

string_t RocketMode::tf_projectile_rocket;
VTableHook<decltype(&RocketMode::PlayerRunCommandHook)> RocketMode::sPlayerRunCommandHook;
VTableHook<decltype(&RocketMode::SetOwnerEntityHook)> RocketMode::sSetOwnerEntityHook;

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
    mSendTables(nullptr),
    mTFBaseRocketClass(nullptr),
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
    mServerGameEnts = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, nullptr);

    IPlayerInfoManager* playerInfoManager = (IPlayerInfoManager*)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, nullptr);
    if (playerInfoManager)
    {
        mGlobals = playerInfoManager->GetGlobalVars();
    }

    if (!mVEngineServer || !mServer || !mServerTools || !mCvar || !mServerGameEnts || !mGlobals)
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
    sSetOwnerEntityHook.Unhook();
    sPlayerRunCommandHook.Unhook();
}

void RocketMode::LevelInit(const char* pMapName)
{
    CBaseEntity* ent = mServerTools->CreateEntityByName("tf_projectile_rocket");
    if (ent)
    {
        tf_projectile_rocket = BaseEntityHelpers::GetClassname(ent);
        if (!sSetOwnerEntityHook.GetThisPtr())
        {
#ifdef SDK_COMPAT
            constexpr int Offset = ;
#else
            constexpr int Offset = 18;
#endif
            sSetOwnerEntityHook.Hook(ent, Offset, this, &RocketMode::SetOwnerEntityHook);
        }
        mServerTools->RemoveEntityImmediate(ent);
    }
}

void RocketMode::LevelShutdown()
{
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
    mClientStates[clientIndex].Reset();
}

void RocketMode::OnEntitySpawned(CBaseEntity* pEntity)
{
    // can't assert here because we need an entity spawned in order to
    // get the value of tf_projectile_rocket in the first place.
    //assert(tf_projectile_rocket);

    // blazing fast early out if not a tf_projectile_rocket.
    const string_t classname = BaseEntityHelpers::GetClassname(pEntity);
    if (!tf_projectile_rocket || (classname != tf_projectile_rocket))
    {
        return;
    }

    // if not networked
    edict_t* edict = mServerGameEnts->BaseEntityToEdict(pEntity);
    if (!edict)
    {
        return;
    }

    // if no owner
    CBaseHandle ownerEntHandle = BaseEntityHelpers::GetOwnerEntity(pEntity);
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

    if (ClientHelpers::SetViewEntity(client, edict))
    {
        State& state = mClientStates[ownerClientIndex];
        if (CBaseEntity* rocketEnt = EntityHelpers::HandleToEnt(state.rocket, mServerTools))
        {
            BaseEntityHelpers::SetLocalAngularVelocity(rocketEnt, QAngle(0.0f, 0.0f, 0.0f));
        }

        state.rocket = pEntity->GetRefEHandle();
        state.owner = ownerEntHandle;
        state.initialSpeed = 0.0f;

        CBaseEntity* ownerEnt = mServerTools->GetBaseEntityByEntIndex(ownerEntIndex);
        assert(ownerEnt);

        BaseEntityHelpers::AddFlag(ownerEnt, FL_FROZEN, mServerGameEnts, mVEngineServer);
    }
}

void RocketMode::OnEntityDeleted(CBaseEntity* pEntity)
{
    // if not "tf_projectile_rocket"
    const string_t classname = BaseEntityHelpers::GetClassname(pEntity);
    if (!tf_projectile_rocket || (classname != tf_projectile_rocket))
    {
        return;
    }

    DetachFromRocket(pEntity);
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

    if (state.initialSpeed == 0.0f)
    {
        const Vector& localVelocity = BaseEntityHelpers::GetLocalVelocity(rocketEnt);
        state.initialSpeed = VectorLength(localVelocity);
    }

    const bool left = (ucmd->buttons & IN_MOVELEFT) != 0;
    const bool right = (ucmd->buttons & IN_MOVERIGHT) != 0;
    const bool up = (ucmd->buttons & IN_FORWARD) != 0;
    const bool down = (ucmd->buttons & IN_BACK) != 0;

    constexpr float turnspeed = 150.0f;

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
    BaseEntityHelpers::SetLocalAngularVelocity(rocketEnt, angVel);

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
    const QAngle& localAngVelocity = BaseEntityHelpers::GetLocalAngularVelocity(rocketEnt);
    angRotation += (localAngVelocity * mGlobals->frametime);

    Vector newVelocity;
    AngleVectors(angRotation, newVelocity);
    newVelocity *= state.initialSpeed * 0.5f;

    BaseEntityHelpers::SetLocalVelocity(rocketEnt, newVelocity);
}

void RocketMode::SetOwnerEntityHook(CBaseEntity* owner)
{
    RocketMode* thisPtr = sPlayerRunCommandHook.GetThisPtr();
    CBaseEntity* rocket = reinterpret_cast<CBaseEntity*>(this);
    thisPtr->SetOwnerEntity(rocket, owner);
    sSetOwnerEntityHook.CallOriginalFn(this, owner);
}

void RocketMode::SetOwnerEntity(CBaseEntity* rocket, CBaseEntity* owner)
{
    CBaseHandle ownerHandle = BaseEntityHelpers::GetOwnerEntity(rocket);
    if (ownerHandle != owner->GetRefEHandle())
    {
        DetachFromRocket(rocket);
    }
}
