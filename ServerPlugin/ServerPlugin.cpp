
#include "ServerPlugin.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/inetchannel.h"
#include "sourcesdk/public/inetmessage.h"
#include <string.h>

class ServerPlugin : public IServerPluginCallbacks
{
public:
    ServerPlugin();
    virtual ~ServerPlugin() {}
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    virtual void Unload(void);
    virtual void Pause(void) {}
    virtual void UnPause(void) {}
    virtual const char* GetPluginDescription(void) { return "ServerPlugin"; }
    virtual void LevelInit(char const* pMapName) {}
    virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) {}
    virtual void GameFrame(bool simulating) {}
    virtual void LevelShutdown(void) {}
    virtual void ClientActive(edict_t* pEntity);
    virtual void ClientDisconnect(edict_t* pEntity) {}
    virtual void ClientPutInServer(edict_t* pEntity, char const* playername) {}
    virtual void SetCommandClient(int index) {}
    virtual void ClientSettingsChanged(edict_t* pEdict) {}
    virtual PLUGIN_RESULT ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen) { return PLUGIN_CONTINUE; }
    virtual PLUGIN_RESULT ClientCommand(edict_t* pEntity, const CCommand& args) { return PLUGIN_CONTINUE; }
    virtual PLUGIN_RESULT NetworkIDValidated(const char* pszUserName, const char* pszNetworkID) { return PLUGIN_CONTINUE; }
    virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t* pPlayerEntity, EQueryCvarValueStatus eStatus, const char* pCvarName, const char* pCvarValue) {}
    virtual void OnEdictAllocated(edict_t* edict) {}
    virtual void OnEdictFreed(const edict_t* edict) {}

private:
    IVEngineServer* mVEngineServer;
    IServer* mServer;
};

static ServerPlugin sServerPlugin;

void* CreateInterface(const char* pName, int* pReturnCode)
{
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

ServerPlugin::ServerPlugin() :
    mVEngineServer(nullptr),
    mServer(nullptr)
{
}

bool ServerPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    mVEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
    if (mVEngineServer)
    {
        mServer = mVEngineServer->GetIServer();
    }
    return mVEngineServer && mServer;
}

template<typename T, typename U>
inline T ByteOffsetFromPointer(U pBase, int byte_offset)
{
    return reinterpret_cast<T>((reinterpret_cast<unsigned char*>(pBase) + byte_offset));
}

template<typename T>
struct CUtlMemory
{
    T* m_pMemory;
    int m_nAllocationCount;
    int m_nGrowSize;
};

template<typename T>
struct CUtlVector
{
    CUtlMemory<T> m_Memory;
    int m_Size;
    T* m_pElements;
};

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class IVoiceDataHook;
using INetMessage_ProcessPtr = bool (INetMessage::*)();
using IVoiceDataHook_ProcessPtr = bool (IVoiceDataHook::*)();

class IVoiceDataHook : public INetMessage
{
public:
    bool ProcessHook()
    {
        return (this->*sVoiceDataProcessFn)();
    }

    static bool IsVoiceDataHooked()
    {
        return sVoiceDataProcessFn;
    }

    static void RegisterProcessHook(INetMessage* NetMsg)
    {
        if (!sVoiceDataProcessFn)
        {
            void** voiceDataVtable = *(void***)NetMsg;

            
            INetMessage_ProcessPtr ProcessPtr = (INetMessage_ProcessPtr&)voiceDataVtable[ProcessOffset];
            IVoiceDataHook_ProcessPtr NewProcessPtr = &IVoiceDataHook::ProcessHook;

            DWORD oldProtect;
            DWORD tempProtect;
            int ret = VirtualProtect(&voiceDataVtable[ProcessOffset], 4, PAGE_EXECUTE_READWRITE, &oldProtect);
            memcpy(&voiceDataVtable[ProcessOffset], &NewProcessPtr, 4);
            VirtualProtect(&voiceDataVtable[ProcessOffset], 4, oldProtect, &tempProtect);

            sVoiceDataVTable = voiceDataVtable;
            sVoiceDataProcessFn = ProcessPtr;
        }
    }

    static void UnregisterProcessHook()
    {
        if (sVoiceDataProcessFn)
        {
            void** voiceDataVtable = sVoiceDataVTable;
            void* processSlot = &voiceDataVtable[ProcessOffset];

            DWORD oldProtect;
            DWORD tempProtect;
            VirtualProtect(processSlot, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
            memcpy(processSlot, &sVoiceDataProcessFn, 4);
            VirtualProtect(processSlot, 4, oldProtect, &tempProtect);

            sVoiceDataProcessFn = nullptr;
            sVoiceDataVTable = nullptr;
        }
    }

private:
    static constexpr int ProcessOffset = 3;
    static void** sVoiceDataVTable;
    static INetMessage_ProcessPtr sVoiceDataProcessFn;
};

void** IVoiceDataHook::sVoiceDataVTable = nullptr;
INetMessage_ProcessPtr IVoiceDataHook::sVoiceDataProcessFn = nullptr;

void ServerPlugin::Unload(void)
{
    IVoiceDataHook::UnregisterProcessHook();
}

void ServerPlugin::ClientActive(edict_t* pEntity)
{
    if (!IVoiceDataHook::IsVoiceDataHooked())
    {
        const int entIndex = mVEngineServer->IndexOfEdict(pEntity);
        IClient* client = mServer->GetClient(entIndex - 1);
        INetChannel* netChannel = client->GetNetChannel();
        if (netChannel)
        {
            //netChannel->RegisterMessage(nullptr);

            CUtlVector<INetMessage*>* pNetMessages = ByteOffsetFromPointer<CUtlVector<INetMessage*>*>(netChannel, 9000);
            for (int i = 0; i < pNetMessages->m_Size; ++i)
            {
                INetMessage* msg = pNetMessages->m_pElements[i];
                const char* name = msg->GetName();
                if (!strcmp(name, "clc_VoiceData"))
                {
                    IVoiceDataHook::RegisterProcessHook(msg);
                }
            }
        }
    }
}
