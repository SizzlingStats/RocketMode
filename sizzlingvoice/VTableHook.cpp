
#include "VTableHook.h"

#include <string.h>

#ifdef _WIN32

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#else

#include <sys/mman.h>

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#define PAGE_EXECUTE_READWRITE (PROT_READ | PROT_WRITE | PROT_EXEC)

#include <stdio.h>
typedef unsigned long ADDRTYPE;
typedef unsigned int DWORD;

static bool VirtualProtect(const void* addr, size_t len, int prot, unsigned int* prev)
{
    ADDRTYPE p = (ADDRTYPE)addr & ~(PAGESIZE - 1);
    int ret = mprotect((void*)p, (ADDRTYPE)addr - p + len, prot);
    if (ret == 0)
    {
        // just make something up. this seems like a probable previous value.
        *prev = PROT_READ | PROT_EXEC;
    }
    return ret == 0;
}

#endif

FnPtr EditVTable(FnPtr* vtable, int slot, const FnPtr* replacementFn)
{
    FnPtr* entry = &vtable[slot];
    FnPtr prevFn = vtable[slot];

    DWORD oldProtect;
    VirtualProtect(entry, sizeof(FnPtr), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(entry, replacementFn, sizeof(FnPtr));
    VirtualProtect(entry, sizeof(FnPtr), oldProtect, &oldProtect);

    return prevFn;
}
