
#pragma once
#if 0
#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/public/tier1/bitbuf.h"
#include "sourcehelpers/VAudioCeltCodecManager.h"
#include "WavFile.h"

class INetMessage;
class Vector;
struct VoiceFxState;
struct edict_t;
class CStaticPropMgr;
class IBaseFileSystem;
class IServerGameEnts;
class IPlayerInfoManager;
class IServerGameClients;

class SizzlingVoiceFx
{
public:
    SizzlingVoiceFx();

    bool Init();
    void Shutdown();

    void LevelInit();
    void LevelShutdown();

    void ClientActive(edict_t* edict);
    void ClientDisconnect(edict_t* edict);

    bool ProcessVoiceData(INetMessage* VoiceDataNetMsg);
    
private:
    void ProcessVoiceData(VoiceFxState* clientState, bf_read voiceData, int numEncodedBits, bool sirenFx);
    int GetClosestBotSlot(const Vector& position);

private:
    VoiceFxState* mFxStates[MAX_PLAYERS];
    VAudioCeltCodecManager mCeltCodecManager;
    WavFile mSpeakerIR;

    IVEngineServer* mVEngineServer;
    IServer* mServer;
    IServerGameClients* mServerGameClients;
    IPlayerInfoManager* mPlayerInfoManager;
    IServerGameEnts* mServerGameEnts;
    CStaticPropMgr* mStaticPropMgr;
    IBaseFileSystem* mFileSystem;
};
#endif