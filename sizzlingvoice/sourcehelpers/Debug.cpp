
#include "Debug.h"
#include "base/platform.h"
#include <assert.h>

Debug::MsgFn* Debug::Msg;

void Debug::Initialize()
{
    Platform::HModule hTier0 = Platform::GetModuleHandle("tier0");

    Debug::Msg = (MsgFn*)Platform::GetProcAddress(hTier0, "Msg");
}
