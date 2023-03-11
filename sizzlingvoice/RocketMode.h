
#pragma once

#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/public/string_t.h"
#include "sourcesdk/public/basehandle.h"
#include "VTableHook.h"

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

class RocketMode
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
    bool ModifyRocketAngularPrecision();

    bool PlayerRunCommandHook(CUserCmd* ucmd, IMoveHelper* moveHelper);
    void PlayerRunCommand(CBaseEntity* player, CUserCmd* ucmd, IMoveHelper* moveHelper);

private:
    static string_t tf_projectile_rocket;
    static VTableHook<decltype(&PlayerRunCommandHook)> sPlayerRunCommandHook;

private:
    IVEngineServer* mVEngineServer;
    IServer* mServer;
    IServerTools* mServerTools;
    IServerGameDLL* mServerGameDll;
    ICvar* mCvar;
    IServerGameEnts* mServerGameEnts;
    CGlobalVars* mGlobals;

    IConVar* mSendTables;
    ServerClass* mTFBaseRocketClass;

    struct State
    {
        CBaseHandle rocket;
        CBaseHandle owner;
        float initialSpeed;
    };
    State mClientStates[MAX_PLAYERS];
};
