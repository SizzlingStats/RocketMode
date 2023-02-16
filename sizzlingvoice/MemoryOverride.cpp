
#include "MemoryOverride.h"
#include "sourcesdk/public/tier0/memalloc.h"
#include <new>
#include <assert.h>

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static HMODULE sTier0;
static IMemAlloc* sMemAlloc;

namespace MemoryOverride
{
    void Init()
    {
        assert(!sTier0);
        sTier0 = LoadLibraryA("tier0");
        assert(sTier0);

        sMemAlloc = *(IMemAlloc**)GetProcAddress(sTier0, "g_pMemAlloc");
        assert(sMemAlloc);
    }

    void Shutdown()
    {
        sMemAlloc = nullptr;
        FreeLibrary(sTier0);
        sTier0 = nullptr;
    }
}

void* operator new(std::size_t count)
{
    return sMemAlloc->Alloc(count);
}

void* operator new[](std::size_t count)
{
    return sMemAlloc->Alloc(count);
}

void* operator new(std::size_t, std::align_val_t)
{
    assert(false);
    return nullptr;
}

void* operator new[](std::size_t, std::align_val_t)
{
    assert(false);
    return nullptr;
}

void* operator new(std::size_t count, const std::nothrow_t&) noexcept
{
    return sMemAlloc->Alloc(count);
}

void* operator new[](std::size_t count, const std::nothrow_t&) noexcept
{
    return sMemAlloc->Alloc(count);
}

void* operator new(std::size_t, std::align_val_t, const std::nothrow_t&) noexcept
{
    assert(false);
    return nullptr;
}

void* operator new[](std::size_t, std::align_val_t, const std::nothrow_t&) noexcept
{
    assert(false);
    return nullptr;
}

void operator delete(void* ptr) noexcept
{
    return sMemAlloc->Free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    return sMemAlloc->Free(ptr);
}

void operator delete(void* ptr, std::align_val_t al) noexcept
{
    assert(false);
}

void operator delete[](void* ptr, std::align_val_t al) noexcept
{
    assert(false);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    return sMemAlloc->Free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept
{
    return sMemAlloc->Free(ptr);
}

void operator delete(void* ptr, std::size_t sz, std::align_val_t al) noexcept
{
    assert(false);
}

void operator delete[](void* ptr, std::size_t sz, std::align_val_t al) noexcept
{
    assert(false);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    return sMemAlloc->Free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    return sMemAlloc->Free(ptr);
}

void operator delete(void*, std::align_val_t, const std::nothrow_t&) noexcept
{
    assert(false);
}

void operator delete[](void*, std::align_val_t, const std::nothrow_t&) noexcept
{
    assert(false);
}
