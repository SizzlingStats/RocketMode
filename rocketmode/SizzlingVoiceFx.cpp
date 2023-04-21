
#include "SizzlingVoiceFx.h"
#if 0
#include "sourcesdk/public/bitvec.h"
#include "sourcesdk/public/edict.h"

#include "dsp/phaser.h"
#include "dsp/bitcrush.h"
#include "dsp/alienwah.h"
#include "dsp/autotalent.h"
#include "dsp/delay.h"
#include "dsp/convolution.h"

#include <xmmintrin.h>
#include <pmmintrin.h>

struct VoiceFxState
{
    VoiceFxState(IVAudioVoiceCodec* codec, const float* kernel, int kernelSamples) :
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

    ~VoiceFxState()
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

void VoiceFxState::ApplyFx(float* samples, int numSamples)
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

void VoiceFxState::SirenFx(float* samples, int numSamples)
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

static ConVar* sSizzVoiceEnabled;
static ConVar* sSizzVoiceAutotune;
static ConVar* sSizzVoiceWah;
static ConVar* sSizzVoicePhaser;
static ConVar* sSizzVoiceBitCrush;
static ConVar* sSizzVoicePositionalSteamID;
static ConVar* sSizzVoicePositional;
static ConVar* sSizzVoiceSirenFx;

SizzlingVoiceFx::SizzlingVoiceFx() :
    mFxStates(),
    mCeltCodecManager(),
    mSpeakerIR(),
    mServerGameClients(nullptr),
    mPlayerInfoManager(nullptr),
    mServerGameEnts(nullptr),
    mStaticPropMgr(nullptr),
    mFileSystem(nullptr)
{
}

bool SizzlingVoiceFx::Init()
{
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

    mFileSystem = (IBaseFileSystem*)interfaceFactory(BASEFILESYSTEM_INTERFACE_VERSION, nullptr);
    if (!mFileSystem)
    {
        return false;
    }

    AutoTalent::GlobalInit();

    if (!mSpeakerIR.Load("addons/ir_siren.wav", mFileSystem))
    {
        return false;
    }

    sSizzVoiceEnabled = CVarHelper::CreateConVar("sizz_voice_enabled", "1");
    sSizzVoiceAutotune = CVarHelper::CreateConVar("sizz_voice_autotune", "0");
    sSizzVoiceWah = CVarHelper::CreateConVar("sizz_voice_wah", "0");
    sSizzVoicePhaser = CVarHelper::CreateConVar("sizz_voice_phaser", "1");
    sSizzVoiceBitCrush = CVarHelper::CreateConVar("sizz_voice_bitcrush", "0");
    sSizzVoicePositionalSteamID = CVarHelper::CreateConVar("sizz_voice_positional_steamid", "");
    sSizzVoicePositional = CVarHelper::CreateConVar("sizz_voice_positional", "0",
        "0 - Default non positional voice.\n"
        "1 - All player voices are positional.\n"
        "2 - Voice is emitted from the closest bot to the listener. (sizz_voice_positional_steamid)\n");
    sSizzVoiceSirenFx = CVarHelper::CreateConVar("sizz_voice_sirenfx", "3");

    return false;
}

void SizzlingVoiceFx::Shutdown()
{
    for (VoiceFxState*& state : mFxStates)
    {
        delete state;
        state = nullptr;
    }

    AutoTalent::GlobalShutdown();

    mCeltCodecManager.Release();

    CVarHelper::DestroyConVar(sSizzVoiceEnabled);
    CVarHelper::DestroyConVar(sSizzVoiceAutotune);
    CVarHelper::DestroyConVar(sSizzVoiceWah);
    CVarHelper::DestroyConVar(sSizzVoicePhaser);
    CVarHelper::DestroyConVar(sSizzVoiceBitCrush);
    CVarHelper::DestroyConVar(sSizzVoicePositionalSteamID);
    CVarHelper::DestroyConVar(sSizzVoicePositional);
    CVarHelper::DestroyConVar(sSizzVoiceSirenFx);
}

int gSpeakerEntIndex;

void SizzlingVoiceFx::LevelInit()
{
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

void SizzlingVoiceFx::LevelShutdown()
{
}

void SizzlingVoiceFx::ClientActive(edict_t* edict)
{
    const int entIndex = edict->m_EdictIndex;
    const int clientIndex = entIndex - 1;

    assert(!mFxStates[clientIndex]);
    delete mFxStates[clientIndex];

    const float* kernel = reinterpret_cast<const float*>(mSpeakerIR.Samples());
    const int numSamples = mSpeakerIR.NumSamples();
    assert(mSpeakerIR.Format() == WavFile::WAVE_FORMAT_IEEE_FLOAT);
    mFxStates[clientIndex] = new VoiceFxState(mCeltCodecManager.CreateVoiceCodec(), kernel, numSamples);
}

void SizzlingVoiceFx::ClientDisconnect(edict_t* edict)
{
    const int entIndex = edict->m_EdictIndex;
    const int clientIndex = entIndex - 1;

    // can delete null here if the client had no net channel (bot/other).
    delete mFxStates[clientIndex];
    mFxStates[clientIndex] = nullptr;
}

bool SizzlingVoiceFx::ProcessVoiceData(INetMessage* VoiceDataNetMsg)
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

void SizzlingVoiceFx::ProcessVoiceData(VoiceFxState* clientState, bf_read voiceData, int numEncodedBits, bool sirenFx)
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

int SizzlingVoiceFx::GetClosestBotSlot(const Vector& position)
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
#endif