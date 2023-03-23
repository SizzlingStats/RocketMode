
#pragma once

#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/public/igameevents.h"
#include "sourcesdk/public/string_t.h"
#include "sourcesdk/public/basehandle.h"
#include "VTableHook.h"
#include "base/math.h"

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
class CBaseEntity;
class IVEngineServer;
class IServer;
class IServerTools;
class IServerGameDLL;
class ICvar;
class CGlobalVars;
struct edict_t;
class CBaseHandle;
class CUserCmd;
class IMoveHelper;
class IConVar;
class ServerClass;
class IServerGameEnts;
class IEngineSound;
class CGameRules;

class RocketMode : public IGameEventListener2
{
public:
    RocketMode();

    bool Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    void Shutdown();

    void LevelInit(const char* pMapName);
    void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax);
    void GameFrame(bool simulating);
    void LevelShutdown();

    void ClientConnect();

    void ClientActive(edict_t* pEntity);
    void ClientDisconnect(edict_t* pEntity);

    void OnEntityDeleted(CBaseEntity* pEntity);

private:
    void AttachToRocket(CBaseEntity* rocketEnt);
    void DetachFromRocket(CBaseEntity* rocketEnt);

    bool ModifyRocketAngularPrecision();

    bool PlayerRunCommandHook(CUserCmd* ucmd, IMoveHelper* moveHelper);
    void PlayerRunCommand(CBaseEntity* player, CUserCmd* ucmd, IMoveHelper* moveHelper);

    void SetOwnerEntityHook(CBaseEntity* owner);
    void SetOwnerEntity(CBaseEntity* rocket, CBaseEntity* newOwner);

    void RocketSpawn(CBaseEntity* rocket);

    void RocketChangeTeamHook(int team);
    void RocketChangeTeam(CBaseEntity* rocket, int oldTeam);

    DECL_INHERITED_DESTRUCTOR(IGameEventListener2);
    virtual void FireGameEvent(IGameEvent* event) override;

private:
    static string_t tf_projectile_rocket;
    static VTableHook<decltype(&RocketMode::PlayerRunCommandHook)> sPlayerRunCommandHook;

    // CTFProjectile_Rocket hooks 
    static VTableHook<decltype(&RocketMode::SetOwnerEntityHook)> sSetOwnerEntityHook;
    static VTableHook<decltype(&RocketMode::RocketChangeTeamHook)> sRocketChangeTeamHook;

private:
    IVEngineServer* mVEngineServer;
    IServer* mServer;
    IServerTools* mServerTools;
    IServerGameDLL* mServerGameDll;
    ICvar* mCvar;
    IServerGameEnts* mServerGameEnts;
    CGlobalVars* mGlobals;
    IGameEventManager2* mGameEventManager;
    IEngineSound* mEngineSound;

    IConVar* mSendTables;
    ServerClass* mTFBaseRocketClass;
    CGameRules* mGameRules;

    struct State
    {
        void Reset()
        {
            rocket.Term();
            owner.Term();
            initialSpeed = 0.0f;
            rollAngle = 0.0f;
        }

        float UpdateRoll(float dt, float target)
        {
            const float smooth = (target == 0.0f) ? 0.2f : 0.01f;
            rollAngle = Math::ExpSmooth(rollAngle, target, smooth, dt);
            return rollAngle;
        }

        CBaseHandle rocket;
        CBaseHandle owner;
        float initialSpeed = 0.0f;
        float rollAngle = 0.0f;
    };
    State mClientStates[MAX_PLAYERS];
};
