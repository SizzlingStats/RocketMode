
#include "ServerPlugin.h"
#include "sourcesdk/public/bitvec.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/filesystem.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/icvar.h"
#include "sourcesdk/public/inetchannel.h"
#include "sourcesdk/public/inetmessage.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/public/tier1/utlvector.h"
#include "sourcesdk/public/mathlib/vector.h"
#include "sourcesdk/common/protocol.h"
#include "sourcesdk/common/netmessages.h"
#include "sourcesdk/game/server/iplayerinfo.h"
#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/engine/gl_model_private.h"
#include "sourcesdk/public/toolframework/itoolentity.h"
#include "sourcesdk/game/server/entitylist.h"
#include "sourcehelpers/EntityHelpers.h"
#include "sourcehelpers/StaticPropMgr.h"
#include "sourcehelpers/ValveMemAlloc.h"
#include "HookOffsets.h"
#include "VTableHook.h"
#include "sourcehelpers/VAudioCeltCodecManager.h"
#include "dsp/phaser.h"
#include "dsp/bitcrush.h"
#include "dsp/alienwah.h"
#include "dsp/autotalent.h"
#include "dsp/delay.h"
#include "dsp/convolution.h"
#include "base/math.h"
#include "sourcehelpers/CVarHelper.h"
#include "sourcehelpers/ClientHelpers.h"
#include "sourcehelpers/Debug.h"
#include "sourcehelpers/VScriptHelpers.h"
#include "WavFile.h"
#include "RocketMode.h"
#include "SizzLauncherSpawner.h"
#include <string.h>
#include <float.h>

struct ClientState
{
    ClientState(IVAudioVoiceCodec* codec, const float* kernel, int kernelSamples) :
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

        mAutoTalent.InitInstance();

        mConvolution.Init(kernel, kernelSamples, 512);
        mDelay.Init(0.1f, 0.1f, 22050.0f);
        mDelay2.Init(0.132f, 0.07f, 22050.0f);

        // vaudio_celt 22050Hz 16-bit mono
        constexpr int celtQuality = 3;
        codec->Init(celtQuality);
    }

    ~ClientState()
    {
        mAutoTalent.DestroyInstance();
        mVoiceCodec->Release();
    }

    void ApplyFx(float* samples, int numSamples);
    void SirenFx(float* samples, int numSamples);

    AlienWah mAlienWah;
    Phaser mPhaser;
    BitCrush mBitCrush;
    AutoTalent mAutoTalent;
    Convolution mConvolution;
    Delay mDelay;
    Delay mDelay2;
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
    virtual void LevelInit(char const* pMapName);
    virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) {}
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

    bool ProcessVoiceDataHook()
    {
        ServerPlugin* thisPtr = sProcessVoiceDataHook.GetThisPtr();
        if (thisPtr->ProcessVoiceData(reinterpret_cast<INetMessage*>(this)))
        {
            return sProcessVoiceDataHook.CallOriginalFn(this);
        }
        return true;
    }

    bool ProcessVoiceData(INetMessage* VoiceDataNetMsg);
    void ProcessVoiceData(ClientState* clientState, bf_read voiceData, int numEncodedBits, bool sirenFx);
    int GetClosestBotSlot(const Vector& position);

    bool IsProximityHearingClientHook(int index);

private:
    IVEngineServer* mVEngineServer;
    IServer* mServer;
    IServerGameClients* mServerGameClients;
    IPlayerInfoManager* mPlayerInfoManager;
    IServerGameEnts* mServerGameEnts;
    CStaticPropMgr* mStaticPropMgr;
    IServerTools* mServerTools;
    IServerGameDLL* mServerGameDll;
    IBaseFileSystem* mFileSystem;

    CVarHelper mCvarHelper;

    VAudioCeltCodecManager mCeltCodecManager;

    ClientState* mClientState[MAX_PLAYERS];
    WavFile mSpeakerIR;
    RocketMode mRocketMode;
    SizzLauncherSpawner mSizzLauncherSpawner;

    static VTableHook<decltype(&ServerPlugin::ProcessVoiceDataHook)> sProcessVoiceDataHook;
    static VTableHook<decltype(&ServerPlugin::IsProximityHearingClientHook)> sIsProximityHearingClientHook;
};

VTableHook<decltype(&ServerPlugin::ProcessVoiceDataHook)> ServerPlugin::sProcessVoiceDataHook;
VTableHook<decltype(&ServerPlugin::IsProximityHearingClientHook)> ServerPlugin::sIsProximityHearingClientHook;

static ServerPlugin sServerPlugin;

static ConVar* sSizzVoiceEnabled;
static ConVar* sSizzVoiceAutotune;
static ConVar* sSizzVoiceWah;
static ConVar* sSizzVoicePhaser;
static ConVar* sSizzVoiceBitCrush;
static ConVar* sSizzVoicePositionalSteamID;
static ConVar* sSizzVoicePositional;
static ConVar* sSizzVoiceSirenFx;

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
    mServerGameClients(nullptr),
    mPlayerInfoManager(nullptr),
    mServerGameEnts(nullptr),
    mStaticPropMgr(nullptr),
    mServerTools(nullptr),
    mServerGameDll(nullptr),
    mFileSystem(nullptr),
    mCvarHelper(),
    mCeltCodecManager(),
    mClientState(),
    mSpeakerIR(),
    mRocketMode()
{
}

bool ServerPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    Debug::Initialize();

    if (!ValveMemAlloc::Init())
    {
        return false;
    }

    if (!mCeltCodecManager.Init())
    {
        return false;
    }

    mVEngineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    if (mVEngineServer)
    {
        mServer = mVEngineServer->GetIServer();
    }

    mServerGameClients = (IServerGameClients*)gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, nullptr);
    if (!mServerGameClients)
    {
        return false;
    }

    mPlayerInfoManager = (IPlayerInfoManager*)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, nullptr);
    if (!mPlayerInfoManager)
    {
        return false;
    }

    mServerGameEnts = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, nullptr);
    if (!mServerGameEnts)
    {
        return false;
    }

    mStaticPropMgr = (CStaticPropMgr*)interfaceFactory(INTERFACEVERSION_STATICPROPMGR_SERVER, nullptr);
    if (!mStaticPropMgr)
    {
        return false;
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

    mFileSystem = (IBaseFileSystem*)interfaceFactory(BASEFILESYSTEM_INTERFACE_VERSION, nullptr);
    if (!mFileSystem)
    {
        return false;
    }

    if (!VScriptHelpers::Initialize(interfaceFactory))
    {
        return false;
    }

    ICvar* cvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr);
    if (!cvar || !mCvarHelper.Init(cvar))
    {
        return false;
    }

    CGlobalEntityList* entityList = mServerTools->GetEntityList();
    if (!entityList)
    {
        return false;
    }
    entityList->m_entityListeners.AddToTail(&sEntityListener);

    AutoTalent::GlobalInit();

    mCvarHelper.UnhideAllCVars();
    sSizzVoiceEnabled = mCvarHelper.CreateConVar("sizz_voice_enabled", "1");
    sSizzVoiceAutotune = mCvarHelper.CreateConVar("sizz_voice_autotune", "0");
    sSizzVoiceWah = mCvarHelper.CreateConVar("sizz_voice_wah", "0");
    sSizzVoicePhaser = mCvarHelper.CreateConVar("sizz_voice_phaser", "1");
    sSizzVoiceBitCrush = mCvarHelper.CreateConVar("sizz_voice_bitcrush", "0");
    sSizzVoicePositionalSteamID = mCvarHelper.CreateConVar("sizz_voice_positional_steamid", "");
    sSizzVoicePositional = mCvarHelper.CreateConVar("sizz_voice_positional", "0",
        "0 - Default non positional voice.\n"
        "1 - All player voices are positional.\n"
        "2 - Voice is emitted from the closest bot to the listener. (sizz_voice_positional_steamid)\n");
    sSizzVoiceSirenFx = mCvarHelper.CreateConVar("sizz_voice_sirenfx", "3");

    if (!mSpeakerIR.Load("addons/ir_siren.wav", mFileSystem))
    {
        return false;
    }

    mRocketMode.Init(interfaceFactory, gameServerFactory);
    mSizzLauncherSpawner.Init(interfaceFactory, gameServerFactory);

    mVEngineServer->ServerCommand("exec sizzlingvoice/sizzlingvoice.cfg\n");

    return mServer && sSizzVoiceEnabled;
}

void ServerPlugin::Unload(void)
{
    mRocketMode.Shutdown();

    mCvarHelper.DestroyConVar(sSizzVoiceEnabled);
    mCvarHelper.DestroyConVar(sSizzVoiceAutotune);
    mCvarHelper.DestroyConVar(sSizzVoiceWah);
    mCvarHelper.DestroyConVar(sSizzVoicePhaser);
    mCvarHelper.DestroyConVar(sSizzVoiceBitCrush);
    mCvarHelper.DestroyConVar(sSizzVoicePositionalSteamID);
    mCvarHelper.DestroyConVar(sSizzVoicePositional);
    mCvarHelper.DestroyConVar(sSizzVoiceSirenFx);

    for (ClientState*& state : mClientState)
    {
        delete state;
        state = nullptr;
    }

    AutoTalent::GlobalShutdown();

    CGlobalEntityList* entityList = mServerTools->GetEntityList();
    if (entityList)
    {
        entityList->m_entityListeners.FindAndFastRemove(&sEntityListener);
    }

    VScriptHelpers::Shutdown();

    mCeltCodecManager.Release();
    sIsProximityHearingClientHook.Unhook();
    sProcessVoiceDataHook.Unhook();

    ValveMemAlloc::Release();
}

int gSpeakerEntIndex;

void ServerPlugin::LevelInit(char const* pMapName)
{
    mRocketMode.LevelInit(pMapName);
    mSizzLauncherSpawner.LevelInit(pMapName);

    mVEngineServer->ServerCommand("exec sizzlingvoice/sizzlingvoice.cfg\n");

    //EntityHelpers::PrintAllServerClassTables(mServerGameDll);
    //EntityHelpers::PrintServerClassTables(mServerGameDll, "CTFProjectile_Rocket");

    const int numStaticProps = mStaticPropMgr->m_StaticProps.m_Size;
    for (int i = 0; i < numStaticProps; ++i)
    {
        CStaticProp& staticProp = mStaticPropMgr->m_StaticProps.m_pElements[i];
        if (!strcmp(staticProp.m_pModel->strName.m_pString, "models/props_spytech/siren001.mdl"))
        {
            CBaseEntity* ent = mServerTools->CreateEntityByName("env_sprite");
            mServerTools->DispatchSpawn(ent);
            mServerTools->SetMoveType(ent, MOVETYPE_NOCLIP, MOVECOLLIDE_DEFAULT);
            mServerTools->SetKeyValue(ent, "origin", staticProp.m_Origin);

            edict_t* edict = mServerGameEnts->BaseEntityToEdict(ent);
            gSpeakerEntIndex = mVEngineServer->IndexOfEdict(edict);
            break;
        }
    }
}

void ServerPlugin::GameFrame(bool simulating)
{
    mRocketMode.GameFrame(simulating);
}

void ServerPlugin::LevelShutdown()
{
    mRocketMode.LevelShutdown();
}

static const INetMessage* GetCLCVoiceData(INetChannel* channel)
{
    // excerpt from CNetChan. Used so we can get to m_NetMessages
    struct CNetChanHack
    {
        INetChannelHandler* m_MessageHandler;   // who registers and processes messages
        CUtlVector<INetMessage*> m_NetMessages; // list of registered message
    };

    INetChannelHandler* messageHandler = channel->GetMsgHandler();
    INetChannelHandler** searchPtr = reinterpret_cast<INetChannelHandler**>(channel);
    while (*searchPtr != messageHandler)
    {
        searchPtr += 1;
    }
    CNetChanHack* hack = reinterpret_cast<CNetChanHack*>(searchPtr);
    
    const int numMessages = hack->m_NetMessages.m_Size;
    INetMessage** messages = hack->m_NetMessages.m_pElements;
    for (int i = 0; i < numMessages; ++i)
    {
        const INetMessage* msg = messages[i];
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
    const int entIndex = pEntity->m_EdictIndex;
    const int clientIndex = entIndex - 1;
    IClient* client = mServer->GetClient(clientIndex);

    BasePlayerHelpers::InitializeOffsets(mServerTools->GetBaseEntityByEntIndex(entIndex));
    ClientHelpers::InitializeOffsets(client, mVEngineServer);

    mRocketMode.ClientActive(pEntity);

    INetChannel* netChannel = client->GetNetChannel();
    if (!netChannel)
    {
        return;
    }

    if (!sIsProximityHearingClientHook.GetThisPtr())
    {
        //client->IsProximityHearingClient(0);
        sIsProximityHearingClientHook.Hook(client, HookOffsets::IsProximityHearingClient, this, &ServerPlugin::IsProximityHearingClientHook);
    }

    if (!sProcessVoiceDataHook.GetThisPtr())
    {
        const INetMessage* msg = GetCLCVoiceData(netChannel);
        assert(msg);

        sProcessVoiceDataHook.Hook(msg, HookOffsets::ProcessVoiceData, this, &ServerPlugin::ProcessVoiceDataHook);
    }

    assert(!mClientState[clientIndex]);
    delete mClientState[clientIndex];

    const float* kernel = reinterpret_cast<const float*>(mSpeakerIR.Samples());
    const int numSamples = mSpeakerIR.NumSamples();
    assert(mSpeakerIR.Format() == WavFile::WAVE_FORMAT_IEEE_FLOAT);
    mClientState[clientIndex] = new ClientState(mCeltCodecManager.CreateVoiceCodec(), kernel, numSamples);
}

void ServerPlugin::ClientDisconnect(edict_t* pEntity)
{
    const int entIndex = pEntity->m_EdictIndex;
    const int clientIndex = entIndex - 1;

    // can delete null here if the client had no net channel (bot/other).
    delete mClientState[clientIndex];
    mClientState[clientIndex] = nullptr;

    mRocketMode.ClientDisconnect(pEntity);
}

PLUGIN_RESULT ServerPlugin::ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen)
{
    mRocketMode.ClientConnect();
    return PLUGIN_CONTINUE;
}

PLUGIN_RESULT ServerPlugin::ClientCommand(edict_t* pEntity, const CCommand& args)
{
    if (mSizzLauncherSpawner.ClientCommand(pEntity, args))
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

int ServerPlugin::GetClosestBotSlot(const Vector& position)
{
    CBitVec<ABSOLUTE_PLAYER_LIMIT> playerbits;
    const bool usePotentialAudibleSet = true;
    mVEngineServer->Message_DetermineMulticastRecipients(usePotentialAudibleSet, position, playerbits);

    int slot = -1;
    float shortestDistSqr = FLT_MAX;
    const int numClients = mServer->GetClientCount();
    for (int i = 0; i < numClients; ++i)
    {
        if (!playerbits.Get(i))
        {
            continue;
        }

        const int entIndex = i + 1;
        edict_t* edict = mVEngineServer->PEntityOfEntIndex(entIndex);
        if (!edict)
        {
            continue;
        }

        IPlayerInfo* playerInfo = mPlayerInfoManager->GetPlayerInfo(edict);
        if (!playerInfo ||
            !playerInfo->IsFakeClient() ||
            playerInfo->IsDead() ||
            playerInfo->IsHLTV() ||
            playerInfo->IsReplay())
        {
            continue;
        }

        Vector earPos;
        mServerGameClients->ClientEarPosition(edict, &earPos);
        const float distSqr = position.DistToSqr(earPos);
        constexpr float maxAudibleDistanceSqr = 2800.0f * 2800.0f;
        if ((distSqr < maxAudibleDistanceSqr) && (distSqr < shortestDistSqr))
        {
            shortestDistSqr = distSqr;
            slot = i;
        }
    }
    return slot;
}

bool ServerPlugin::ProcessVoiceData(INetMessage* VoiceDataNetMsg)
{
    if (!sSizzVoiceEnabled->IsEnabled())
    {
        return true;
    }

    INetChannel* netChannel = VoiceDataNetMsg->GetNetChannel();
    IClient* client = static_cast<IClient*>(netChannel->GetMsgHandler());
    const int playerSlot = client->GetPlayerSlot();
    const int sourceEntIndex = playerSlot + 1;
    ClientState* clientState = mClientState[playerSlot];

    CLC_VoiceData* voiceData = static_cast<CLC_VoiceData*>(VoiceDataNetMsg);

    bool positionalEnabled = false;
    const int positionalMode = sSizzVoicePositional->GetInt();
    if ((positionalMode > 1) && (sSizzVoicePositionalSteamID->m_StringLength > 1))
    {
        const CSteamID* steamId = mVEngineServer->GetClientSteamIDByPlayerIndex(sourceEntIndex);
        const CSteamID positionalSteamId = strtoull(sSizzVoicePositionalSteamID->m_pszString, nullptr, 10);
        positionalEnabled = (steamId && (*steamId == positionalSteamId));
    }

    const bool sirenFx = positionalEnabled && (positionalMode == 3) && (gSpeakerEntIndex > 0);
    ProcessVoiceData(clientState, voiceData->m_DataIn, voiceData->m_nLength, sirenFx);

    if (positionalEnabled)
    {
        bf_read dataCopy = voiceData->m_DataIn;

        char voiceDataBuffer[4096];
        int bitsRead = dataCopy.ReadBitsClamped(voiceDataBuffer, voiceData->m_nLength);

        SVC_VoiceData svcVoiceData;
        svcVoiceData.m_nFromClient = playerSlot;
        svcVoiceData.m_bProximity = true;
        svcVoiceData.m_nLength = bitsRead;
        svcVoiceData.m_DataOut = voiceDataBuffer;

        if (positionalMode == 2)
        {
            const int numClients = mServer->GetClientCount();
            for (int i = 0; i < numClients; ++i)
            {
                IClient* destClient = mServer->GetClient(i);
                if (destClient)
                {
                    const bool isHltvOrReplay = destClient->IsHLTV() || destClient->IsReplay();
                    if (isHltvOrReplay || !destClient->IsFakeClient())
                    {
                        const int entIndex = isHltvOrReplay ? sourceEntIndex : (i + 1);
                        edict_t* edict = mVEngineServer->PEntityOfEntIndex(entIndex);
                        if (edict)
                        {
                            Vector earPos;
                            mServerGameClients->ClientEarPosition(edict, &earPos);
                            const int closestBotIndex = GetClosestBotSlot(earPos);
                            if (closestBotIndex != -1)
                            {
                                svcVoiceData.m_nFromClient = closestBotIndex;
                                destClient->SendNetMsg(svcVoiceData);
                            }
                        }
                    }
                }
            }
            return false;
        }
        else if (sirenFx)
        {
            svcVoiceData.m_nFromClient = gSpeakerEntIndex - 1;
            const int numClients = mServer->GetClientCount();
            for (int i = 0; i < numClients; ++i)
            {
                IClient* destClient = mServer->GetClient(i);
                if (destClient)
                {
                    destClient->SendNetMsg(svcVoiceData);
                }
            }
            return false;
        }
    }
    return true;
}

#include <xmmintrin.h>
#include <pmmintrin.h>

void ServerPlugin::ProcessVoiceData(ClientState* clientState, bf_read voiceData, int numEncodedBits, bool sirenFx)
{
    const unsigned int oldDtz = _MM_GET_DENORMALS_ZERO_MODE();
    const unsigned int oldFtz = _MM_GET_FLUSH_ZERO_MODE();
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    assert(voiceData.GetNumBitsLeft() >= numEncodedBits);

    // Pad up to 4 byte boundary.
    // bf_write complains if totalBytesAvailable is not dword aligned because it writes in dwords.
    // TODO fix this
    const int totalBytesAvailable = (voiceData.TotalBytesAvailable() + 3) & ~3;
    bf_write voiceOutputWriter((void*)voiceData.GetBasePointer(), totalBytesAvailable);
    voiceOutputWriter.SeekToBit(voiceData.GetNumBitsRead());

    constexpr int celtFrameSizeBytes = 64;
    constexpr int celtFrameSizeBits = celtFrameSizeBytes * 8;
    constexpr int celtDecodedFrameSizeSamples = 512;

    char encodedFrame[celtFrameSizeBytes];
    int16_t decodedFrame[celtDecodedFrameSizeSamples];
    float samples[celtDecodedFrameSizeSamples];

    IVAudioVoiceCodec* voiceCodec = clientState->mVoiceCodec;
    while (numEncodedBits >= celtFrameSizeBits)
    {
        numEncodedBits -= celtFrameSizeBits;
        voiceData.ReadBits(encodedFrame, celtFrameSizeBits);
        assert(!voiceData.IsOverflowed());

        const int numSamples = voiceCodec->Decompress(encodedFrame, sizeof(encodedFrame), (char*)decodedFrame, sizeof(decodedFrame));
        assert(numSamples == celtDecodedFrameSizeSamples);

        for (int i = 0; i < celtDecodedFrameSizeSamples; ++i)
        {
            samples[i] = decodedFrame[i] / 32768.0f;
        }
        clientState->ApplyFx(samples, celtDecodedFrameSizeSamples);
        if (sirenFx)
        {
            clientState->SirenFx(samples, celtDecodedFrameSizeSamples);
        }
        for (int i = 0; i < celtDecodedFrameSizeSamples; ++i)
        {
            const int32_t expandedSample = static_cast<int32_t>(samples[i] * 32768.0f);
            decodedFrame[i] = static_cast<int16_t>(Math::Max(-32768, Math::Min(expandedSample, 32767)));
        }

        // TODO: bFinal = true when the last byte of the sound data is a 0?
        const int bytesWritten = voiceCodec->Compress((char*)decodedFrame, celtDecodedFrameSizeSamples, encodedFrame, sizeof(encodedFrame), false);
        assert(bytesWritten == celtFrameSizeBytes);

        // write back to net message
        voiceOutputWriter.WriteBits(encodedFrame, celtFrameSizeBits);
    }
    assert(numEncodedBits == 0);

    _MM_SET_FLUSH_ZERO_MODE(oldDtz);
    _MM_SET_DENORMALS_ZERO_MODE(oldFtz);
}

void ClientState::ApplyFx(float* samples, int numSamples)
{
    if (sSizzVoiceAutotune->IsEnabled())
    {
        mAutoTalent.ProcessBuffer(samples, numSamples);
    }

    if (sSizzVoiceWah->IsEnabled())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            samples[i] = mAlienWah.Process(samples[i]);
        }
    }

    if (sSizzVoicePhaser->IsEnabled())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            samples[i] = mPhaser.Update(samples[i]);
        }
    }

    {
        mBitsRadians += 0.05f;
        constexpr float PI_2 = (2.0f * 3.14159f);
        if (mBitsRadians >= PI_2)
        {
            mBitsRadians -= PI_2;
        }
        float lfo = (Math::Sin(mBitsRadians) * 0.5f) + 0.5f;
        lfo = Math::Clamp(lfo, 0.0f, 1.0f);
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
        float lfo = (Math::Sin(mRateRadians) * 0.5f) + 0.5f;
        lfo = Math::Clamp(lfo, 0.0f, 1.0f);
        float value = (lfo * 500.0f) + 7000.0f;
        mBitCrush.Rate(value);
    }

    if (sSizzVoiceBitCrush->IsEnabled())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float normalizedSample = (samples[i] * 0.5f) + 0.5f;
            normalizedSample = mBitCrush.Process(normalizedSample);
            samples[i] = (normalizedSample - 0.5f) * 2.0f;
        }
    }
}

void ClientState::SirenFx(float* samples, int numSamples)
{
    const int mode = sSizzVoiceSirenFx->GetInt();
    if (mode > 0)
    {
        mConvolution.Process(samples, numSamples);
        if (mode > 1)
        {
            mDelay.Process(samples, numSamples);
            if (mode > 2)
            {
                mDelay2.Process(samples, numSamples);
            }
        }
    }
}

bool ServerPlugin::IsProximityHearingClientHook(int index)
{
    if (sSizzVoicePositional->GetInt() == 1)
    {
        return true;
    }
    return sIsProximityHearingClientHook.CallOriginalFn(this, index);
}
