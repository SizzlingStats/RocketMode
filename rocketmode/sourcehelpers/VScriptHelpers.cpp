
#include "VScriptHelpers.h"

#define USE_VSCRIPT 0
#if USE_VSCRIPT

#include "sourcesdk/public/vscript/ivscript.h"
#include "../VTableHook.h"
#include "../HookOffsets.h"
#include <assert.h>

// Assuming only one vscript instance
class ScriptManagerShim
{
public:
    void Initialize(IScriptManager* scriptManager)
    {
        if (!sCreateVMHook.GetThisPtr())
        {
            sCreateVMHook.Hook(scriptManager, HookOffsets::CreateVM, this, &ScriptManagerShim::CreateVMHook);
        }
        if (!sDestroyVMHook.GetThisPtr())
        {
            sDestroyVMHook.Hook(scriptManager, HookOffsets::DestroyVM, this, &ScriptManagerShim::DestroyVMHook);
        }
    }

    void Shutdown()
    {
        mServerScriptVM = nullptr;
        sDestroyVMHook.Unhook();
        sCreateVMHook.Unhook();
    }

    IScriptVM* CreateVMHook(ScriptLanguage_t language)
    {
        IScriptVM* vm = sCreateVMHook.CallOriginalFn(this, language);
        ScriptManagerShim* thisPtr = sCreateVMHook.GetThisPtr();
        assert(!thisPtr->mServerScriptVM);
        thisPtr->mServerScriptVM = vm;
        return vm;
    }

    void DestroyVMHook(IScriptVM* vm)
    {
        ScriptManagerShim* thisPtr = sDestroyVMHook.GetThisPtr();
        assert(vm == thisPtr->mServerScriptVM);
        thisPtr->mServerScriptVM = nullptr;
        sDestroyVMHook.CallOriginalFn(this, vm);
    }

    IScriptVM* GetVM() const
    {
        return mServerScriptVM;
    }

private:
    IScriptVM* mServerScriptVM = nullptr;

    static VTableHook<decltype(&CreateVMHook)> sCreateVMHook;
    static VTableHook<decltype(&DestroyVMHook)> sDestroyVMHook;
};

VTableHook<decltype(&ScriptManagerShim::CreateVMHook)> ScriptManagerShim::sCreateVMHook;
VTableHook<decltype(&ScriptManagerShim::DestroyVMHook)> ScriptManagerShim::sDestroyVMHook;
static ScriptManagerShim gScriptManagerShim;

bool VScriptHelpers::Initialize(CreateInterfaceFn interfaceFactory)
{
    IScriptManager* vScript = (IScriptManager*)interfaceFactory(VSCRIPT_INTERFACE_VERSION, nullptr);
    if (vScript)
    {
        gScriptManagerShim.Initialize(vScript);
        return true;
    }
    return false;
}

void VScriptHelpers::Shutdown()
{
    gScriptManagerShim.Shutdown();
}

IScriptVM* VScriptHelpers::GetVM()
{
    return gScriptManagerShim.GetVM();
}

#else

bool VScriptHelpers::Initialize(CreateInterfaceFn interfaceFactory)
{
    return true;
}

void VScriptHelpers::Shutdown()
{
}

IScriptVM* VScriptHelpers::GetVM()
{
    return nullptr;
}

#endif