
#include "VAudioCeltCodecManager.h"
#include "base/platform.h"
#include <assert.h>

#ifdef _WIN32
#define VAUDIO_MODULE "vaudio_celt.dll"
#else
#define VAUDIO_MODULE "vaudio_celt.so"
#endif

VAudioCeltCodecManager::VAudioCeltCodecManager() :
    mVAudioCeltDll(nullptr),
    mIVAudioVoiceCodecCreateInterfaceFunc(nullptr)
{
}

bool VAudioCeltCodecManager::Init()
{
    mVAudioCeltDll = Platform::LoadLibrary(VAUDIO_MODULE);
    assert(mVAudioCeltDll);

    mIVAudioVoiceCodecCreateInterfaceFunc = (CreateInterfaceFn)Platform::GetProcAddress(mVAudioCeltDll, "CreateInterface");
    assert(mIVAudioVoiceCodecCreateInterfaceFunc);

    return true;
}

void VAudioCeltCodecManager::Release()
{
    Platform::FreeLibrary(mVAudioCeltDll);
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
