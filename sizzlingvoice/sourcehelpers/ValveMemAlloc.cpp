
#include "ValveMemAlloc.h"
#include "base/platform.h"
#include <assert.h>

class IMemAlloc;
IMemAlloc* g_pMemAlloc;

bool ValveMemAlloc::Init()
{
    assert(!g_pMemAlloc);

    Platform::HModule hTier0 = Platform::GetModuleHandle("tier0.dll");
    assert(hTier0);

    g_pMemAlloc = *reinterpret_cast<IMemAlloc**>(Platform::GetProcAddress(hTier0, "g_pMemAlloc"));
    assert(g_pMemAlloc);

    return g_pMemAlloc;
}

void ValveMemAlloc::Release()
{
    g_pMemAlloc = nullptr;
}
