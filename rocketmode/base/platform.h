
#pragma once

namespace Platform
{
    using HModule = void*;

    HModule LoadLibrary(const char* name);
    void FreeLibrary(HModule module);

    HModule GetModuleHandle(const char* name);
    void* GetProcAddress(HModule module, const char* name);
}
