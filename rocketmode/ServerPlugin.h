
#pragma once

#include "sourcesdk/public/engine/iserverplugin.h"

#ifdef _MSC_VER
#define DLL_EXPORT extern "C" __declspec( dllexport )
#else
#define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
#endif

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);

struct edict_t;
class CCommand;
typedef int QueryCvarCookie_t;

class IServerPluginCallbacks
{
public:
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) = 0;
    virtual void Unload(void) = 0;
    virtual void Pause(void) = 0;
    virtual void UnPause(void) = 0;
    virtual const char* GetPluginDescription(void) = 0;
    virtual void LevelInit(char const* pMapName) = 0;
    virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) = 0;
    virtual void GameFrame(bool simulating) = 0;
    virtual void LevelShutdown(void) = 0;
    virtual void ClientActive(edict_t* pEntity) = 0;
    virtual void ClientDisconnect(edict_t* pEntity) = 0;
    virtual void ClientPutInServer(edict_t* pEntity, char const* playername) = 0;
    virtual void SetCommandClient(int index) = 0;
    virtual void ClientSettingsChanged(edict_t* pEdict) = 0;
    virtual PLUGIN_RESULT ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen) = 0;
    virtual PLUGIN_RESULT ClientCommand(edict_t* pEntity, const CCommand& args) = 0;
    virtual PLUGIN_RESULT NetworkIDValidated(const char* pszUserName, const char* pszNetworkID) = 0;
    virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t* pPlayerEntity, EQueryCvarValueStatus eStatus, const char* pCvarName, const char* pCvarValue) = 0;
    virtual void OnEdictAllocated(edict_t* edict) = 0;
    virtual void OnEdictFreed(const edict_t* edict) = 0;
};

// interface return status
enum
{
    IFACE_OK = 0,
    IFACE_FAILED
};

DLL_EXPORT void* CreateInterface(const char* pName, int* pReturnCode);
