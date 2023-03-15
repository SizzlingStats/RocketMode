
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

class RocketMode : public IGameEventListener2
{
public:
    RocketMode();
    ~RocketMode();

    bool Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    void Shutdown();

    void LevelInit(const char* pMapName);
    void LevelShutdown();

    void ClientConnect();

    void ClientActive(edict_t* pEntity);
    void ClientDisconnect(edict_t* pEntity);

    void OnEntitySpawned(CBaseEntity* pEntity);
    void OnEntityDeleted(CBaseEntity* pEntity);

private:
    void DetachFromRocket(CBaseEntity* rocketEnt);

    bool ModifyRocketAngularPrecision();

    bool PlayerRunCommandHook(CUserCmd* ucmd, IMoveHelper* moveHelper);
    void PlayerRunCommand(CBaseEntity* player, CUserCmd* ucmd, IMoveHelper* moveHelper);

    void SetOwnerEntityHook(CBaseEntity* owner);
    void SetOwnerEntity(CBaseEntity* rocket, CBaseEntity* owner);

    virtual void IGameEventListener2_Destructor() override;
    virtual void FireGameEvent(IGameEvent* event) override;

private:
    static string_t tf_projectile_rocket;
    static VTableHook<decltype(&PlayerRunCommandHook)> sPlayerRunCommandHook;
    static VTableHook<decltype(&SetOwnerEntityHook)> sSetOwnerEntityHook;

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
            rollAngle = Math::Lerp(target, rollAngle, Math::ExpDecay2(dt));
            return rollAngle;
        }

        CBaseHandle rocket;
        CBaseHandle owner;
        float initialSpeed = 0.0f;
        float rollAngle = 0.0f;
    };
    State mClientStates[MAX_PLAYERS];
};
