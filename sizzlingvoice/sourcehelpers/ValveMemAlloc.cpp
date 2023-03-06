
#include "ValveMemAlloc.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <assert.h>

class IMemAlloc;
IMemAlloc* g_pMemAlloc;

bool ValveMemAlloc::Init()
{
    assert(!g_pMemAlloc);

    HMODULE hTier0 = GetModuleHandleA("tier0.dll");
    assert(hTier0);

    g_pMemAlloc = *reinterpret_cast<IMemAlloc**>(GetProcAddress(hTier0, "g_pMemAlloc"));
    assert(g_pMemAlloc);

    return g_pMemAlloc;
}

void ValveMemAlloc::Release()
{
    g_pMemAlloc = nullptr;
}
