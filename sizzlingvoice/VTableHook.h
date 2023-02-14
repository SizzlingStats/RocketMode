
#pragma once

template<class T> struct get_class;
template<class T, class R> struct get_class<R T::*> { using type = T; };

unsigned char* EditVTable(unsigned char** vtable, int slot, unsigned char* replacementFn);

template<typename HookMemberFnPtr>
class VTableHook
{
    using HookClassType = typename get_class<HookMemberFnPtr>::type;

public:
    VTableHook() :
        mHookThisPtr(nullptr)
    {
    }

    // thisPtr - instance of class with vtable to hook.
    // vtableSlot - vtable slot to replace.
    // hookThisPtr - instance of class to receive hooked call.
    // hookFn - function to call on hookThisPtr.
    void Hook(const void* thisPtr, int vtableSlot, HookClassType* hookThisPtr, HookMemberFnPtr hookFn)
    {
        if (mHookThisPtr)
        {
            return;
        }

        mHookThisPtr = hookThisPtr;
        mHookMemberFnPtr = hookFn;

        unsigned char** vtable = *(unsigned char***)thisPtr;
        mHookedFnSlot = &vtable[vtableSlot];
        mOriginalFn = EditVTable(vtable, vtableSlot, (unsigned char*&)hookFn);
    }

    void Unhook()
    {
        if (!mHookThisPtr)
        {
            return;
        }
        mHookThisPtr = nullptr;
        EditVTable(mHookedFnSlot, 0, mOriginalFn);
    }

    template<typename T, typename... Args>
    auto CallOriginalFn(T* thisPtr, Args... args)
    {
        return (thisPtr->*((HookMemberFnPtr&)mOriginalFn))(args...);
    }

    HookClassType* GetThisPtr()
    {
        return mHookThisPtr;
    }

private:
    HookClassType* mHookThisPtr;
    HookMemberFnPtr mHookMemberFnPtr;

    unsigned char** mHookedFnSlot;
    unsigned char* mOriginalFn;
};
