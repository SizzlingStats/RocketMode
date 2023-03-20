
#include "ValveMemAlloc.h"
#include "base/platform.h"
#include <assert.h>

class IMemAlloc;
IMemAlloc* g_pMemAlloc;

#ifdef _WIN32
#define TIER0_MODULE "tier0.dll"
#else
#define TIER0_MODULE "libtier0_srv.so"
#endif

bool ValveMemAlloc::Init()
{
    assert(!g_pMemAlloc);

    Platform::HModule hTier0 = Platform::GetModuleHandle(TIER0_MODULE);
    assert(hTier0);

    g_pMemAlloc = *reinterpret_cast<IMemAlloc**>(Platform::GetProcAddress(hTier0, "g_pMemAlloc"));
    assert(g_pMemAlloc);

    return g_pMemAlloc;
}

void ValveMemAlloc::Release()
{
    g_pMemAlloc = nullptr;
}
