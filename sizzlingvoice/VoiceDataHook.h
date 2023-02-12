
#pragma once

class INetMessage;

class __declspec(novtable) IVoiceDataHook
{
public:
    virtual void ProcessVoiceData(INetMessage* VoiceDataNetMsg) = 0;
};

namespace VoiceDataHook
{
    void Hook(const INetMessage* VoiceDataNetMsg, IVoiceDataHook* hook);
    void Unhook();

    inline bool IsHooked()
    {
        extern IVoiceDataHook* sVoiceDataHook;
        return sVoiceDataHook;
    }
}
