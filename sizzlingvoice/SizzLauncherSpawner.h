
#pragma once

#include "VTableHook.h"

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
class IServerGameClients;
class IServerTools;
//class IServerGameDLL;
//class IServerGameEnts;
//class IVEngineServer;
struct edict_t;
class CCommand;
class CBaseEntity;
class ConVar;

class SizzLauncherSpawner
{
public:
    SizzLauncherSpawner();
    ~SizzLauncherSpawner();

    bool Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    void Shutdown();

    void LevelInit(const char* pMapName);

    bool ClientCommand(edict_t* pEntity, const CCommand& args);

private:
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
    ConVar* mTfDroppedWeaponLifetime;
    //IServerGameDLL* mServerGameDll;
    //IServerGameEnts* mServerGameEnts;
    //IVEngineServer* mVEngineServer;
};
