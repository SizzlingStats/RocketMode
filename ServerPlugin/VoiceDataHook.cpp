
#include "VoiceDataHook.h"
#include "sourcesdk/public/inetmessage.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using INetMessage_ProcessPtr = bool (INetMessage::*)();

namespace VoiceDataHook
{
    static constexpr int ProcessOffset = 3;
    static unsigned char** sVoiceDataVTable;
    static INetMessage_ProcessPtr sVoiceDataProcessFn;
    IVoiceDataHook* sVoiceDataHook;

    class HookInternal : public INetMessage
    {
    public:
        bool ProcessHook()
        {
            VoiceDataHook::sVoiceDataHook->ProcessVoiceData(this);
            return (this->*sVoiceDataProcessFn)();
        }
    };

    void Hook(const INetMessage* VoiceDataNetMsg, IVoiceDataHook* hook)
    {
        if (sVoiceDataVTable)
        {
            return;
        }

        unsigned char** voiceDataVtable = *(unsigned char***)VoiceDataNetMsg;
        unsigned char** processSlot = &voiceDataVtable[ProcessOffset];

        sVoiceDataVTable = voiceDataVtable;
        sVoiceDataProcessFn = *(INetMessage_ProcessPtr*)processSlot;
        sVoiceDataHook = hook;

        DWORD oldProtect;
        VirtualProtect(processSlot, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
        auto NewProcessPtr = &HookInternal::ProcessHook;
        memcpy(processSlot, &NewProcessPtr, 4);
        VirtualProtect(processSlot, 4, oldProtect, &oldProtect);
    }

    void Unhook()
    {
        if (!sVoiceDataProcessFn)
        {
            return;
        }

        unsigned char** processSlot = &sVoiceDataVTable[ProcessOffset];

        DWORD oldProtect;
        VirtualProtect(processSlot, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(processSlot, &sVoiceDataProcessFn, 4);
        VirtualProtect(processSlot, 4, oldProtect, &oldProtect);

        sVoiceDataVTable = nullptr;
        sVoiceDataProcessFn = nullptr;
        sVoiceDataHook = nullptr;
    }
}
