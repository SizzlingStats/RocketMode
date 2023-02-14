
#include "VTableHook.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

unsigned char* EditVTable(unsigned char** vtable, int slot, unsigned char* replacementFn)
{
    unsigned char** entry = &vtable[slot];
    unsigned char* prevFn = vtable[slot];

    DWORD oldProtect;
    VirtualProtect(entry, sizeof(unsigned char*), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(entry, &replacementFn, sizeof(unsigned char*));
    VirtualProtect(entry, sizeof(unsigned char*), oldProtect, &oldProtect);

    return prevFn;
}
