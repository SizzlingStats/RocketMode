
#include "ServerPlugin.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/icvar.h"
#include "sourcesdk/public/toolframework/itoolentity.h"
#include "sourcesdk/game/server/entitylist.h"
#include "sourcehelpers/EntityHelpers.h"
#include "sourcehelpers/ValveMemAlloc.h"
#include "HookOffsets.h"

#include "sourcehelpers/CVarHelper.h"
#include "sourcehelpers/ClientHelpers.h"
#include "sourcehelpers/Debug.h"
#include "sourcehelpers/VScriptHelpers.h"
#include "sourcehelpers/VStdlibRandom.h"
#include "RocketMode.h"
#include "SizzLauncherSpawner.h"
#include <string.h>

class CGameRules;

class ServerPlugin : public IServerPluginCallbacks
{
public:
    ServerPlugin();
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    virtual void Unload(void);
    virtual void Pause(void) {}
    virtual void UnPause(void) {}
    virtual const char* GetPluginDescription(void) { return "SizzlingVoice v0.9.4 by SizzlingCalamari - Compiled on " __DATE__; }
    virtual void LevelInit(char const* pMapName);
    virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax);
    virtual void GameFrame(bool simulating);
    virtual void LevelShutdown(void);
    virtual void ClientActive(edict_t* pEntity);
    virtual void ClientDisconnect(edict_t* pEntity);
    virtual void ClientPutInServer(edict_t* pEntity, char const* playername) {}
    virtual void SetCommandClient(int index) {}
    virtual void ClientSettingsChanged(edict_t* pEdict) {}
    virtual PLUGIN_RESULT ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen);
    virtual PLUGIN_RESULT ClientCommand(edict_t* pEntity, const CCommand& args);
    virtual PLUGIN_RESULT NetworkIDValidated(const char* pszUserName, const char* pszNetworkID) { return PLUGIN_CONTINUE; }
    virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t* pPlayerEntity, EQueryCvarValueStatus eStatus, const char* pCvarName, const char* pCvarValue) {}
    virtual void OnEdictAllocated(edict_t* edict) {}
    virtual void OnEdictFreed(const edict_t* edict) {}

    void OnEntityCreated(CBaseEntity* pEntity);
    void OnEntitySpawned(CBaseEntity* pEntity);
    void OnEntityDeleted(CBaseEntity* pEntity);

private:
    IVEngineServer* mVEngineServer;
    IServer* mServer;
    IServerTools* mServerTools;
    IServerGameDLL* mServerGameDll;

    RocketMode mRocketMode;
    SizzLauncherSpawner mSizzLauncherSpawner;
};

static ServerPlugin sServerPlugin;

void* CreateInterface(const char* pName, int* pReturnCode)
{
    static bool sInitialized = false;
    if (sInitialized)
    {
        return nullptr;
    }
    sInitialized = true;

    if (!strcmp(pName, "ISERVERPLUGINCALLBACKS003"))
    {
        if (pReturnCode)
        {
            *pReturnCode = IFACE_OK;
        }
        return &sServerPlugin;
    }
    if (pReturnCode)
    {
        *pReturnCode = IFACE_FAILED;
    }
    return nullptr;
}

class ServerPluginEntityListener : public IEntityListener
{
public:
    virtual void OnEntityCreated(CBaseEntity* pEntity) { sServerPlugin.OnEntityCreated(pEntity); }
    virtual void OnEntitySpawned(CBaseEntity* pEntity) { sServerPlugin.OnEntitySpawned(pEntity); }
    virtual void OnEntityDeleted(CBaseEntity* pEntity) { sServerPlugin.OnEntityDeleted(pEntity); }
};

static ServerPluginEntityListener sEntityListener;

ServerPlugin::ServerPlugin() :
    mVEngineServer(nullptr),
    mServer(nullptr),
    mServerTools(nullptr),
    mServerGameDll(nullptr),
    mRocketMode(),
    mSizzLauncherSpawner()
{
}

bool ServerPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    Debug::Initialize();
    VStdlibRandom::Initialize();

    if (!ValveMemAlloc::Init())
    {
        return false;
    }

    if (!VScriptHelpers::Initialize(interfaceFactory))
    {
        return false;
    }

    mVEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    if (mVEngineServer)
    {
        mServer = mVEngineServer->GetIServer();
    }

    mServerTools = (IServerTools*)gameServerFactory(VSERVERTOOLS_INTERFACE_VERSION, nullptr);
    if (!mServerTools)
    {
        return false;
    }

    mServerGameDll = (IServerGameDLL*)gameServerFactory(INTERFACEVERSION_SERVERGAMEDLL, nullptr);
    if (!mServerGameDll)
    {
        return false;
    }

    ICvar* cvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr);
    if (!cvar || !CVarHelper::Initialize(cvar))
    {
        return false;
    }

    CGlobalEntityList* entityList = mServerTools->GetEntityList();
    if (!entityList)
    {
        return false;
    }
    entityList->m_entityListeners.AddToTail(&sEntityListener);

    CVarHelper::UnhideAllCVars();

    TFPlayerHelpers::InitializeOffsets(mServerGameDll);

    if (!mRocketMode.Init(interfaceFactory, gameServerFactory))
    {
        return false;
    }

    if (!mSizzLauncherSpawner.Init(interfaceFactory, gameServerFactory))
    {
        return false;
    }

    mVEngineServer->ServerCommand("exec sizzlingvoice/sizzlingvoice.cfg\n");

    return mServer;
}

void ServerPlugin::Unload(void)
{
    mSizzLauncherSpawner.Shutdown();
    mRocketMode.Shutdown();

    CGlobalEntityList* entityList = mServerTools->GetEntityList();
    if (entityList)
    {
        entityList->m_entityListeners.FindAndFastRemove(&sEntityListener);
    }

    VScriptHelpers::Shutdown();
    ValveMemAlloc::Release();
}

void ServerPlugin::LevelInit(char const* pMapName)
{
    mRocketMode.LevelInit(pMapName);
    mSizzLauncherSpawner.LevelInit(pMapName);

    mVEngineServer->ServerCommand("exec sizzlingvoice/sizzlingvoice.cfg\n");
}

void ServerPlugin::ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
    CGameRules* gameRules = nullptr;
    SendProp* gameRulesProp = EntityHelpers::GetProp(mServerGameDll, "CTFGameRulesProxy", "DT_TeamplayRoundBasedRulesProxy", "teamplayroundbased_gamerules_data");
    if (gameRulesProp)
    {
        SendTableProxyFn proxyFn = gameRulesProp->GetDataTableProxyFn();
        if (proxyFn)
        {
            CSendProxyRecipients recp;
            gameRules = reinterpret_cast<CGameRules*>(proxyFn(nullptr, nullptr, nullptr, &recp, 0));
        }
    }
    assert(gameRules);

    mRocketMode.ServerActivate(gameRules);
    mSizzLauncherSpawner.ServerActivate(gameRules);
}

void ServerPlugin::GameFrame(bool simulating)
{
    mRocketMode.GameFrame(simulating);
    mSizzLauncherSpawner.GameFrme(simulating);
}

void ServerPlugin::LevelShutdown()
{
    mRocketMode.LevelShutdown();
    mSizzLauncherSpawner.LevelShutdown();
}

void ServerPlugin::ClientActive(edict_t* pEntity)
{
    const int entIndex = pEntity->m_EdictIndex;
    const int clientIndex = entIndex - 1;
    IClient* client = mServer->GetClient(clientIndex);
    if (client->IsFakeClient())
    {
        return;
    }

    BasePlayerHelpers::InitializeOffsets(mServerTools->GetBaseEntityByEntIndex(entIndex));
    ClientHelpers::InitializeOffsets(client, mVEngineServer);

    mRocketMode.ClientActive(pEntity);
}

void ServerPlugin::ClientDisconnect(edict_t* pEntity)
{
    mRocketMode.ClientDisconnect(pEntity);
}

PLUGIN_RESULT ServerPlugin::ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen)
{
    mRocketMode.ClientConnect();
    return PLUGIN_CONTINUE;
}

PLUGIN_RESULT ServerPlugin::ClientCommand(edict_t* pEntity, const CCommand& args)
{
    if (mRocketMode.ClientCommand(pEntity, args))
    {
        return PLUGIN_STOP;
    }
    else if (mSizzLauncherSpawner.ClientCommand(pEntity, args))
    {
        return PLUGIN_STOP;
    }
    return PLUGIN_CONTINUE;
}

void ServerPlugin::OnEntityCreated(CBaseEntity* pEntity)
{
    BaseEntityHelpers::InitializeOffsets(pEntity);
}

void ServerPlugin::OnEntitySpawned(CBaseEntity* pEntity)
{
}

void ServerPlugin::OnEntityDeleted(CBaseEntity* pEntity)
{
    mRocketMode.OnEntityDeleted(pEntity);
}
