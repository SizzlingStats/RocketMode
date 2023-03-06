
#pragma once

#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/public/string_t.h"
#include "sourcesdk/public/basehandle.h"
#include "VTableHook.h"

class CBaseEntity;
class IVEngineServer;
class IServer;
class IServerTools;
struct edict_t;
class CBaseHandle;
class CUserCmd;
class IMoveHelper;

class RocketMode
{
public:
    RocketMode();
    ~RocketMode();

    bool Init(IVEngineServer* engineServer, IServer* server, IServerTools* serverTools);
    void Shutdown();

    void LevelInit(const char* pMapName);
    void LevelShutdown();

    void ClientActive(edict_t* pEntity);
    void ClientDisconnect(edict_t* pEntity);

    void OnEntitySpawned(CBaseEntity* pEntity);
    void OnEntityDeleted(CBaseEntity* pEntity);

private:
    bool PlayerRunCommandHook(CUserCmd* ucmd, IMoveHelper* moveHelper);
    void PlayerRunCommand(CBaseEntity* player, CUserCmd* ucmd, IMoveHelper* moveHelper);

private:
    static string_t tf_projectile_rocket;
    static int sClassnameOffset;
    static int sOwnerEntityOffset;
    static int sfFlagsOffset;
    static VTableHook<decltype(&PlayerRunCommandHook)> sPlayerRunCommandHook;

    static string_t GetClassname(CBaseEntity* ent);
    static CBaseHandle GetOwnerEntity(CBaseEntity* ent);

private:
    IVEngineServer* mVEngineServer;
    IServer* mServer;
    IServerTools* mServerTools;

    struct State
    {
        CBaseEntity* rocket;
        CBaseHandle owner;
    };
    State mClientStates[MAX_PLAYERS];
};
