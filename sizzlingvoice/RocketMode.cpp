
#include "RocketMode.h"
#include "sourcesdk/common/netmessages.h"
#include "sourcesdk/game/server/baseentity.h"
#include "sourcesdk/public/basehandle.h"
#include "sourcesdk/public/const.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/iservernetworkable.h"
#include "sourcesdk/public/iserverunknown.h"
#include "sourcesdk/public/toolframework/itoolentity.h"
#include "sourcehelpers/DatamapHelpers.h"
#include "sourcehelpers/EdictChangeHelpers.h"

string_t RocketMode::tf_projectile_rocket;
int RocketMode::sClassnameOffset;
int RocketMode::sOwnerEntityOffset;
int RocketMode::sfFlagsOffset;

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
    mClientStates()
{
}

RocketMode::~RocketMode()
{
}

bool RocketMode::Init(IVEngineServer* engineServer, IServer* server, IServerTools* serverTools)
{
    mVEngineServer = engineServer;
    mServer = server;
    mServerTools = serverTools;
    return true;
}

void RocketMode::Shutdown()
{
}

void RocketMode::LevelInit(const char* pMapName)
{
    CBaseEntity* ent = mServerTools->CreateEntityByName("tf_projectile_rocket");
    if (ent)
    {
        if (!sClassnameOffset)
        {
            sClassnameOffset = DatamapHelpers::GetDatamapVarOffsetFromEnt(ent, "m_iClassname");
            assert(sClassnameOffset > 0);
        }
        if (!sOwnerEntityOffset)
        {
            sOwnerEntityOffset = DatamapHelpers::GetDatamapVarOffsetFromEnt(ent, "m_hOwnerEntity");
            assert(sOwnerEntityOffset > 0);
        }
        if (!sfFlagsOffset)
        {
            sfFlagsOffset = DatamapHelpers::GetDatamapVarOffsetFromEnt(ent, "m_fFlags");
            assert(sfFlagsOffset > 0);
        }

        tf_projectile_rocket = *(string_t*)((char*)ent + sClassnameOffset);
        mServerTools->RemoveEntityImmediate(ent);
    }
}

void RocketMode::LevelShutdown()
{
    tf_projectile_rocket = nullptr;
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

    // TODO: hook g_CommentarySystem.PrePlayerRunCommand as a pre usercmd process to clear weapon flags.
    // TODO: hook g_pGameMovement->ProcessMovement for usercmds to get buttons

    //mServerTools->GetKeyValue()
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
