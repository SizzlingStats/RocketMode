
#pragma once

#include "sourcesdk/public/basehandle.h"
#include "VTableHook.h"
#include <stdint.h>

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
class IServerGameClients;
class IServerTools;
class IPlayerInfoManager;
class IVEngineServer;
class IServer;
class CGlobalVars;
struct edict_t;
class CCommand;
class CBaseEntity;
class ConVar;
class Vector;
class CGameRules;

class SizzLauncherSpawner
{
public:
    SizzLauncherSpawner();
    ~SizzLauncherSpawner();

    bool Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    void Shutdown();

    void LevelInit(const char* pMapName);
    void ServerActivate(CGameRules* gameRules);
    void GameFrme(bool bSimulating);
    void LevelShutdown();

    bool ClientCommand(edict_t* pEntity, const CCommand& args);

private:
    CBaseHandle SpawnLauncher(const Vector& origin);

    void RocketLauncherSpawnHook();
    void RocketLauncherSpawn(CBaseEntity* rocketLauncher);

    void DroppedWeaponSpawnHook();
    void DroppedWeaponSpawn(CBaseEntity* droppedWeapon);

private:
    static VTableHook<decltype(&SizzLauncherSpawner::RocketLauncherSpawnHook)> sRocketLauncherSpawnHook;
    static VTableHook<decltype(&SizzLauncherSpawner::DroppedWeaponSpawnHook)> sDroppedWeaponSpawnHook;

private:
    IServerGameClients* mServerGameClients;
    IServerTools* mServerTools;
    IPlayerInfoManager* mPlayerInfoManager;
    IVEngineServer* mVEngineServer;
    IServer* mServer;
    CGlobalVars* mGlobals;
    CGameRules* mGameRules;

    ConVar* mTfDroppedWeaponLifetime;
    ConVar* mSpawnInitialDelay;
    ConVar* mSpawnInterval;
    ConVar* mSpawnsEnabled;
    ConVar* mSpawnCommandEnabled;

    uint32_t mNextSpawnTick;
    int32_t mRoundState;
};
