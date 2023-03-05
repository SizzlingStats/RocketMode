
#include "vaudioceltcodecmanager.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <assert.h>

VAudioCeltCodecManager::VAudioCeltCodecManager() :
    mVAudioCeltDll(nullptr),
    mIVAudioVoiceCodecCreateInterfaceFunc(nullptr)
{
}

bool VAudioCeltCodecManager::Init()
{
    mVAudioCeltDll = LoadLibrary(TEXT("vaudio_celt.dll"));
    assert(mVAudioCeltDll);

    mIVAudioVoiceCodecCreateInterfaceFunc = (CreateInterfaceFn)GetProcAddress((HMODULE)mVAudioCeltDll, "CreateInterface");
    assert(mIVAudioVoiceCodecCreateInterfaceFunc);

    return true;
}

void VAudioCeltCodecManager::Release()
{
    FreeLibrary((HMODULE)mVAudioCeltDll);
    mVAudioCeltDll = nullptr;
    mIVAudioVoiceCodecCreateInterfaceFunc = nullptr;
}

IVAudioVoiceCodec* VAudioCeltCodecManager::CreateVoiceCodec()
{
    assert(mIVAudioVoiceCodecCreateInterfaceFunc);

    int ret = 0;
    IVAudioVoiceCodec* codec = (IVAudioVoiceCodec*)mIVAudioVoiceCodecCreateInterfaceFunc("vaudio_celt", &ret);
    assert(codec);

    return codec;
}
