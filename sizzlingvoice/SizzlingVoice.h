
#pragma once

#include "sourcesdk/game/shared/shareddefs.h"
#include "VTableHook.h"
#include <stdint.h>

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
class IServer;
class ConVar;
struct edict_t;
class CLC_VoiceData;

class SizzlingVoice
{
public:
    SizzlingVoice();

    bool Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
    void Shutdown();

    void ClientActive(edict_t* pEntity);

private:
    bool CLCVoiceDataProcessHook();
    bool CLCVoiceDataProcess(CLC_VoiceData* clcVoiceData);

    void BroadcastVoiceData(int fromClientIndex, uint8_t* data, int numBytes);

private:
    static VTableHook<decltype(&SizzlingVoice::CLCVoiceDataProcessHook)> sCLCVoiceDataProcessHook;

private:
    IServer* mServer;
    ConVar* mVoiceEnable;

    uint16_t mSourceEntOverride[MAX_PLAYERS];
};
