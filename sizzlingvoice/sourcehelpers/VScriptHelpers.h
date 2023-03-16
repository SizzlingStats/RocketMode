
#pragma once

class IScriptManager;
class IScriptVM;
typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);

namespace VScriptHelpers
{
    bool Initialize(CreateInterfaceFn interfaceFactory);
    void Shutdown();

    IScriptVM* GetVM();
}
