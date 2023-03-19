
#include "platform.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#undef GetModuleHandle
#undef LoadLibrary
#else
#include <dlfcn.h>
#endif

Platform::HModule Platform::LoadLibrary(const char* name)
{
#ifdef _WIN32
    return LoadLibraryA(name);
#else
    return dlopen(name, RTLD_NOW);
#endif
}

void Platform::FreeLibrary(HModule module)
{
#ifdef _WIN32
    ::FreeLibrary(static_cast<HMODULE>(module));
#else
    if (module)
    {
        dlclose(module);
    }
#endif
}

Platform::HModule Platform::GetModuleHandle(const char* name)
{
#ifdef _WIN32
    return ::GetModuleHandleA(name);
#else
    void* handle = dlopen(name, RTLD_NOW);
    if (handle)
    {
        // dec ref count from dlopen.
        dlclose(handle);
    }
    return handle;
#endif
}

void* Platform::GetProcAddress(HModule module, const char* name)
{
#ifdef _WIN32
    return ::GetProcAddress(static_cast<HMODULE>(module), name);
#else
    return dlsym(module, name);
#endif
}
