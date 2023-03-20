
#include "Debug.h"
#include "base/platform.h"
#include <assert.h>

Debug::MsgFn* Debug::Msg;

#ifdef _WIN32
#define TIER0_MODULE "tier0.dll"
#else
#define TIER0_MODULE "libtier0_srv.so"
#endif

void Debug::Initialize()
{
    Platform::HModule hTier0 = Platform::GetModuleHandle(TIER0_MODULE);
    assert(hTier0);

    Debug::Msg = (MsgFn*)Platform::GetProcAddress(hTier0, "Msg");
    assert(Debug::Msg);
}
