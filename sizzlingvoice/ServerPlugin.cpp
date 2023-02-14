
#include "ServerPlugin.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/inetchannel.h"
#include "sourcesdk/public/inetmessage.h"
#include "sourcesdk/common/protocol.h"
#include "sourcesdk/common/netmessages.h"
#include "sourcesdk/game/shared/shareddefs.h"
#include "VTableHook.h"
#include "VAudioCeltCodecManager.h"
#include "dsp/phaser.h"
#include "dsp/bitcrush.h"
#include "dsp/alienwah.h"
#include <string.h>

template<typename T>
inline T Min(T a, T b) { return a <= b ? a : b;  }

template<typename T>
inline T Max(T a, T b) { return a >= b ? a : b; }

struct ClientState
{
    ClientState(IVAudioVoiceCodec* codec) :
        mAlienWah(),
        mPhaser(),
        mBitCrush(4500.0f, 22050.0f, 7.0f),
        mBitsRadians(0.0f),
        mRateRadians(0.0f),
        mVoiceCodec(codec)
    {
        mAlienWah.Init(2.6f, 0.0f, 0.5f, 20);

        mPhaser.Rate(5.0f);
        mPhaser.Depth(0.3f);

        // vaudio_celt 22050Hz 16-bit mono
        constexpr int celtQuality = 3;
        codec->Init(celtQuality);
    }

    ~ClientState()
    {
        mVoiceCodec->Release();
    }

    void ApplyFx(float* samples, int numSamples);

    AlienWah mAlienWah;
    Phaser mPhaser;
    BitCrush mBitCrush;
    float mBitsRadians = 0.0f;
    float mRateRadians = 0.0f;

    IVAudioVoiceCodec* mVoiceCodec;
};

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
    virtual void ClientDisconnect(edict_t* pEntity);
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

    bool IsProximityHearingClientHook(int index)
    {
        return sIsProximityHearingClientHook.CallOriginalFn(this, index);
    }

private:
    IVEngineServer* mVEngineServer;
    IServer* mServer;
    VAudioCeltCodecManager mCeltCodecManager;

    ClientState* mClientState[MAX_PLAYERS];

    static VTableHook<decltype(&ProcessVoiceDataHook)> sProcessVoiceDataHook;
    static VTableHook<decltype(&IsProximityHearingClientHook)> sIsProximityHearingClientHook;
};

VTableHook<decltype(&ServerPlugin::ProcessVoiceDataHook)> ServerPlugin::sProcessVoiceDataHook;
VTableHook<decltype(&ServerPlugin::IsProximityHearingClientHook)> ServerPlugin::sIsProximityHearingClientHook;

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
    mClientState()
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
    for (ClientState*& state : mClientState)
    {
        delete state;
        state = nullptr;
    }
    mCeltCodecManager.Release();
    sIsProximityHearingClientHook.Unhook();
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

static const INetMessage* GetCLCVoiceData(INetChannel* channel)
{
    //channel->RegisterMessage(nullptr);
    constexpr int NetMessagesOffset = 9000;
    CUtlVector<INetMessage*>* pNetMessages = ByteOffsetFromPointer<CUtlVector<INetMessage*>*>(channel, NetMessagesOffset);
    for (int i = 0; i < pNetMessages->m_Size; ++i)
    {
        const INetMessage* msg = pNetMessages->m_pElements[i];
        const int type = msg->GetType();
        if (type == clc_VoiceData)
        {
            return msg;
        }
    }
    return nullptr;
}

void ServerPlugin::ClientActive(edict_t* pEntity)
{
    const int entIndex = mVEngineServer->IndexOfEdict(pEntity);
    IClient* client = mServer->GetClient(entIndex - 1);

    INetChannel* netChannel = client->GetNetChannel();
    if (!netChannel)
    {
        return;
    }

    if (!sIsProximityHearingClientHook.GetThisPtr())
    {
        //client->IsProximityHearingClient(0);
        constexpr int IsProximityHearingClientOffset = 38;
        sIsProximityHearingClientHook.Hook(client, IsProximityHearingClientOffset, this, &ServerPlugin::IsProximityHearingClientHook);
    }

    if (!sProcessVoiceDataHook.GetThisPtr())
    {
        const INetMessage* msg = GetCLCVoiceData(netChannel);
        constexpr int ProcessOffset = 3;
        sProcessVoiceDataHook.Hook(msg, ProcessOffset, this, &ServerPlugin::ProcessVoiceDataHook);
    }

    const int playerSlot = client->GetPlayerSlot();
    assert(!mClientState[playerSlot]);
    delete mClientState[playerSlot];

    mClientState[playerSlot] = new ClientState(mCeltCodecManager.CreateVoiceCodec());
}

void ServerPlugin::ClientDisconnect(edict_t* pEntity)
{
    const int entIndex = mVEngineServer->IndexOfEdict(pEntity);
    IClient* client = mServer->GetClient(entIndex - 1);
    const int playerSlot = client->GetPlayerSlot();

    // can delete null here if the client had no net channel (bot/other).
    delete mClientState[playerSlot];
    mClientState[playerSlot] = nullptr;
}

#define Bits2Bytes(b) ((b+7)>>3)

void ServerPlugin::ProcessVoiceData(INetMessage* VoiceDataNetMsg)
{
    INetChannel* netChannel = VoiceDataNetMsg->GetNetChannel();
    INetChannelHandler* msgHandler = netChannel->GetMsgHandler();
    IClient* client = static_cast<IClient*>(msgHandler);
    const int playerSlot = client->GetPlayerSlot();
    ClientState* clientState = mClientState[playerSlot];

    CLC_VoiceData* voiceData = static_cast<CLC_VoiceData*>(VoiceDataNetMsg);

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
    const int numSamples = clientState->mVoiceCodec->Decompress(compressedData, compressedBytes, (char*)uncompressedData, sizeof(uncompressedData));

    float samples[2048];
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = uncompressedData[i] / 32768.0f;
    }

    clientState->ApplyFx(samples, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const int32_t expandedSample = static_cast<int32_t>(samples[i] * 32768.0f);
        uncompressedData[i] = static_cast<int16_t>(Max(-32768, Min(expandedSample, 32767)));
    }

    // TODO: bFinal = true when the last byte of the sound data is a 0.
    int bytesWritten = clientState->mVoiceCodec->Compress((const char*)uncompressedData, numSamples, compressedData, sizeof(compressedData), false);
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

void ClientState::ApplyFx(float* samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = mAlienWah.Process(samples[i]);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = mPhaser.Update(samples[i]);
    }

    {
        mBitsRadians += 0.05f;
        constexpr float PI_2 = (2.0f * 3.14159f);
        if (mBitsRadians >= PI_2)
        {
            mBitsRadians -= PI_2;
        }
        float lfo = (sinf(mBitsRadians) * 0.5f) + 0.5f;
        if (lfo < 0.0f)
        {
            lfo = 0.0f;
        }
        if (lfo > 1.0f)
        {
            lfo = 1.0f;
        }
        float value = (lfo * 4.0f) + 3.0f;
        mBitCrush.Bits(value);
    }
    {
        mRateRadians += 0.07f;
        constexpr float PI_2 = (2.0f * 3.14159f);
        if (mRateRadians >= PI_2)
        {
            mRateRadians -= PI_2;
        }
        float lfo = (sinf(mRateRadians) * 0.5f) + 0.5f;
        if (lfo < 0.0f)
        {
            lfo = 0.0f;
        }
        if (lfo > 1.0f)
        {
            lfo = 1.0f;
        }
        float value = (lfo * 500.0f) + 7000.0f;
        mBitCrush.Rate(value);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float normalizedSample = (samples[i] * 0.5f) + 0.5f;
        normalizedSample = mBitCrush.Process(normalizedSample);
        samples[i] = (normalizedSample - 0.5f) * 2.0f;
    }
}
