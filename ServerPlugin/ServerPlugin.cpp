
#include "ServerPlugin.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/inetchannel.h"
#include "sourcesdk/public/inetmessage.h"
#include "sourcesdk/common/protocol.h"
#include "sourcesdk/common/netmessages.h"
#include "VAudioCeltCodecManager.h"
#include "dsp/phaser.h"
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
    VAudioCeltCodecManager mCeltCodecManager;
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
    mServer(nullptr),
    mCeltCodecManager()
{
}

bool ServerPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    if (!mCeltCodecManager.Init())
    {
        return false;
    }

    mVEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
    if (mVEngineServer)
    {
        mServer = mVEngineServer->GetIServer();
    }
    return mServer;
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

#define Bits2Bytes(b) ((b+7)>>3)

static Phaser sPhaser;

class IVoiceDataHook : public INetMessage
{
public:
    static IVAudioVoiceCodec* sVoiceCodec;

    bool ProcessHook()
    {
        CLC_VoiceData* voiceData = (CLC_VoiceData*)this;
        bf_read dataInCopy = voiceData->m_DataIn;

        char compressedData[4096];
        int bitsRead = dataInCopy.ReadBitsClamped(compressedData, voiceData->m_nLength);
        if (bitsRead == 0)
        {
            return (this->*sVoiceDataProcessFn)();
        }
        assert(bitsRead == voiceData->m_nLength);

        int16_t uncompressedData[2048];
        const int compressedBytes = Bits2Bytes(bitsRead);
        const int numSamples = sVoiceCodec->Decompress(compressedData, compressedBytes, (char*)uncompressedData, sizeof(uncompressedData));

        // process
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = uncompressedData[i] / 32768.0f;
            sample = sPhaser.Update(sample);

            const int32_t expandedSample = static_cast<int32_t>(sample * 32768.0f);
            uncompressedData[i] = static_cast<int16_t>(max(-32768, min(expandedSample, 32767)));
        }

        char recompressedData[4096];
        int bytesWritten = sVoiceCodec->Compress((const char*)uncompressedData, numSamples, recompressedData, sizeof(recompressedData), false);
        assert(bytesWritten == compressedBytes);

        voiceData->m_DataIn.StartReading(recompressedData, bytesWritten);
        voiceData->m_nLength = bytesWritten * 8;

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
            unsigned char** voiceDataVtable = *(unsigned char***)NetMsg;
            unsigned char** processSlot = &voiceDataVtable[ProcessOffset];

            INetMessage_ProcessPtr ProcessPtr = *(INetMessage_ProcessPtr*)processSlot;
            IVoiceDataHook_ProcessPtr NewProcessPtr = &IVoiceDataHook::ProcessHook;

            DWORD oldProtect;
            VirtualProtect(processSlot, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
            memcpy(processSlot, &NewProcessPtr, 4);
            VirtualProtect(processSlot, 4, oldProtect, &oldProtect);

            sVoiceDataVTable = voiceDataVtable;
            sVoiceDataProcessFn = ProcessPtr;
        }
    }

    static void UnregisterProcessHook()
    {
        if (sVoiceDataProcessFn)
        {
            unsigned char** voiceDataVtable = sVoiceDataVTable;
            unsigned char** processSlot = &voiceDataVtable[ProcessOffset];

            DWORD oldProtect;
            VirtualProtect(processSlot, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
            memcpy(processSlot, &sVoiceDataProcessFn, 4);
            VirtualProtect(processSlot, 4, oldProtect, &oldProtect);

            sVoiceDataProcessFn = nullptr;
            sVoiceDataVTable = nullptr;
        }
    }

private:
    static constexpr int ProcessOffset = 3;
    static unsigned char** sVoiceDataVTable;
    static INetMessage_ProcessPtr sVoiceDataProcessFn;
};

IVAudioVoiceCodec* IVoiceDataHook::sVoiceCodec = nullptr;
unsigned char** IVoiceDataHook::sVoiceDataVTable = nullptr;
INetMessage_ProcessPtr IVoiceDataHook::sVoiceDataProcessFn = nullptr;

void ServerPlugin::Unload(void)
{
    if (IVoiceDataHook::sVoiceCodec)
    {
        IVoiceDataHook::sVoiceCodec->Release();
        IVoiceDataHook::sVoiceCodec = nullptr;
    }
    mCeltCodecManager.Release();

    IVoiceDataHook::UnregisterProcessHook();
}

constexpr int gCeltQuality = 3;

void ServerPlugin::ClientActive(edict_t* pEntity)
{
    if (!IVoiceDataHook::IsVoiceDataHooked())
    {
        sPhaser.Rate(5.0f);
        sPhaser.Depth(0.3f);

        IVoiceDataHook::sVoiceCodec = mCeltCodecManager.CreateVoiceCodec();
        IVoiceDataHook::sVoiceCodec->Init(gCeltQuality);

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
                const int type = msg->GetType();
                if (type == clc_VoiceData)
                {
                    IVoiceDataHook::RegisterProcessHook(msg);
                }
            }
        }
    }
}
