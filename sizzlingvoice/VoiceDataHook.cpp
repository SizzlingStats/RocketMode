
#include "VoiceDataHook.h"
#include "sourcesdk/public/inetmessage.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using INetMessage_ProcessPtr = bool (INetMessage::*)();

static unsigned char* EditVTable(unsigned char** vtable, int slot, unsigned char* replacementFn)
{
    unsigned char** entry = &vtable[slot];
    unsigned char* prevFn = vtable[slot];

    DWORD oldProtect;
    VirtualProtect(entry, sizeof(unsigned char*), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(entry, &replacementFn, sizeof(unsigned char*));
    VirtualProtect(entry, sizeof(unsigned char*), oldProtect, &oldProtect);

    return prevFn;
}

namespace VoiceDataHook
{
    static constexpr int ProcessOffset = 3;
    static unsigned char** sVoiceDataVTable;
    static unsigned char* sVoiceDataProcessFn;
    IVoiceDataHook* sVoiceDataHook;

    class HookInternal : public INetMessage
    {
    public:
        bool ProcessHook()
        {
            VoiceDataHook::sVoiceDataHook->ProcessVoiceData(this);
            return (this->*((INetMessage_ProcessPtr&)sVoiceDataProcessFn))();
        }
    };

    void Hook(const INetMessage* VoiceDataNetMsg, IVoiceDataHook* hook)
    {
        if (sVoiceDataVTable)
        {
            return;
        }

        auto NewProcessPtr = &HookInternal::ProcessHook;
        unsigned char** vtable = *(unsigned char***)VoiceDataNetMsg;
        sVoiceDataProcessFn = EditVTable(vtable, ProcessOffset, (unsigned char*&)NewProcessPtr);

        sVoiceDataVTable = vtable;
        sVoiceDataHook = hook;
    }

    void Unhook()
    {
        if (!sVoiceDataProcessFn)
        {
            return;
        }

        EditVTable(sVoiceDataVTable, ProcessOffset, sVoiceDataProcessFn);

        sVoiceDataVTable = nullptr;
        sVoiceDataProcessFn = nullptr;
        sVoiceDataHook = nullptr;
    }
}
