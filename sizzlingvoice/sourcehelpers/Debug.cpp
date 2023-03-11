
#include "Debug.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <assert.h>

Debug::MsgFn* Debug::Msg;

void Debug::Initialize()
{
    HMODULE hTier0 = GetModuleHandleA("tier0");

    Debug::Msg = (MsgFn*)GetProcAddress(hTier0, "Msg");
}
