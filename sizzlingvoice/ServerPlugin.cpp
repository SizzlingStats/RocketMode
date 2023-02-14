
#include "ServerPlugin.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/inetchannel.h"
#include "sourcesdk/public/inetmessage.h"
#include "sourcesdk/common/protocol.h"
#include "sourcesdk/common/netmessages.h"
#include "VTableHook.h"
#include "VAudioCeltCodecManager.h"
#include "dsp/phaser.h"
#include "dsp/bitcrush.h"
#include <string.h>

template<typename T>
inline T Min(T a, T b) { return a <= b ? a : b;  }

template<typename T>
inline T Max(T a, T b) { return a >= b ? a : b; }

class ServerPlugin : public IServerPluginCallbacks
{
public:
    ServerPlugin();
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

    bool ProcessVoiceDataHook()
    {
        ServerPlugin* thisPtr = sProcessVoiceDataHook.GetThisPtr();
        thisPtr->ProcessVoiceData(reinterpret_cast<INetMessage*>(this));

        return sProcessVoiceDataHook.CallOriginalFn(this);
    }

    void ProcessVoiceData(INetMessage* VoiceDataNetMsg);
    void ApplyFx(float* samples, int numSamples);

private:
    IVEngineServer* mVEngineServer;
    IServer* mServer;
    VAudioCeltCodecManager mCeltCodecManager;
    IVAudioVoiceCodec* mVoiceCodec;

    static VTableHook<decltype(&ProcessVoiceDataHook)> sProcessVoiceDataHook;
};

VTableHook<decltype(&ServerPlugin::ProcessVoiceDataHook)> ServerPlugin::sProcessVoiceDataHook;

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
    mCeltCodecManager(),
    mVoiceCodec(nullptr)
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

void ServerPlugin::Unload(void)
{
    if (mVoiceCodec)
    {
        mVoiceCodec->Release();
        mVoiceCodec = nullptr;
    }
    mCeltCodecManager.Release();
    sProcessVoiceDataHook.Unhook();
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

static Phaser sPhaser;
static BitCrush sBitCrush(4500.0f, 22050.0f, 7.0f);
static float sBitsRadians = 0.0f;
static float sRateRadians = 0.0f;

constexpr int gCeltQuality = 3;

void ServerPlugin::ClientActive(edict_t* pEntity)
{
    if (!sProcessVoiceDataHook.GetThisPtr())
    {
        sPhaser.Rate(5.0f);
        sPhaser.Depth(0.3f);

        mVoiceCodec = mCeltCodecManager.CreateVoiceCodec();
        mVoiceCodec->Init(gCeltQuality);

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
                    constexpr int ProcessOffset = 3;
                    sProcessVoiceDataHook.Hook((unsigned char*)msg, 3, this, &ServerPlugin::ProcessVoiceDataHook);
                }
            }
        }
    }
}

#define Bits2Bytes(b) ((b+7)>>3)

void ServerPlugin::ProcessVoiceData(INetMessage* VoiceDataNetMsg)
{
    CLC_VoiceData* voiceData = (CLC_VoiceData*)VoiceDataNetMsg;

    char compressedData[4096];
    {
        bf_read dataInCopy = voiceData->m_DataIn;
        int bitsRead = dataInCopy.ReadBitsClamped(compressedData, voiceData->m_nLength);
        if (bitsRead == 0)
        {
            return;
        }
        assert(bitsRead == voiceData->m_nLength);
    }

    int16_t uncompressedData[2048];
    const int compressedBytes = Bits2Bytes(voiceData->m_nLength);
    const int numSamples = mVoiceCodec->Decompress(compressedData, compressedBytes, (char*)uncompressedData, sizeof(uncompressedData));

    float samples[2048];
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = uncompressedData[i] / 32768.0f;
    }

    ApplyFx(samples, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const int32_t expandedSample = static_cast<int32_t>(samples[i] * 32768.0f);
        uncompressedData[i] = static_cast<int16_t>(Max(-32768, Min(expandedSample, 32767)));
    }

    int bytesWritten = mVoiceCodec->Compress((const char*)uncompressedData, numSamples, compressedData, sizeof(compressedData), false);
    assert(bytesWritten == compressedBytes);

    {
        int totalBytesAvailable = voiceData->m_DataIn.TotalBytesAvailable();

        // Pad up to 4 byte boundary.
        // bf_write complains if totalBytesAvailable is not dword aligned because it writes in dwords.
        // TODO fix this
        totalBytesAvailable = (totalBytesAvailable + 3) & ~3;

        bf_write writer((void*)voiceData->m_DataIn.GetBasePointer(), totalBytesAvailable);
        writer.SeekToBit(voiceData->m_DataIn.GetNumBitsRead());
        writer.WriteBits(compressedData, bytesWritten * 8);
    }
}

void ServerPlugin::ApplyFx(float* samples, int numSamples)
{
    // process
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = sPhaser.Update(samples[i]);
    }

    {
        sBitsRadians += 0.05f;
        constexpr float PI_2 = (2.0f * 3.14159f);
        if (sBitsRadians >= PI_2)
        {
            sBitsRadians -= PI_2;
        }
        float lfo = (sinf(sBitsRadians) * 0.5f) + 0.5f;
        if (lfo < 0.0f)
        {
            lfo = 0.0f;
        }
        if (lfo > 1.0f)
        {
            lfo = 1.0f;
        }
        float value = (lfo * 4.0f) + 3.0f;
        sBitCrush.Bits(value);
    }
    {
        sRateRadians += 0.07f;
        constexpr float PI_2 = (2.0f * 3.14159f);
        if (sRateRadians >= PI_2)
        {
            sRateRadians -= PI_2;
        }
        float lfo = (sinf(sRateRadians) * 0.5f) + 0.5f;
        if (lfo < 0.0f)
        {
            lfo = 0.0f;
        }
        if (lfo > 1.0f)
        {
            lfo = 1.0f;
        }
        float value = (lfo * 500.0f) + 4500.0f;
        sBitCrush.Rate(value);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float normalizedSample = (samples[i] * 0.5f) + 0.5f;
        normalizedSample = sBitCrush.Process(normalizedSample);
        samples[i] = (normalizedSample - 0.5f) * 2.0f;
    }
}
