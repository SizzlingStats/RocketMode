
#pragma once

template<class T> struct get_class;
template<class T, class R> struct get_class<R T::*> { using type = T; };

struct MemFnPtr
{
    void DummyFunc();

    static constexpr size_t MemFnPtrSize = sizeof(&MemFnPtr::DummyFunc);

    alignas(MemFnPtrSize) unsigned char mData[MemFnPtrSize];
};

MemFnPtr EditVTable(MemFnPtr* vtable, int slot, const MemFnPtr* replacementFn);

template<typename HookMemberFnPtr>
class VTableHook
{
    using HookClassType = typename get_class<HookMemberFnPtr>::type;

public:
    VTableHook() :
        mHookThisPtr(nullptr),
        mHookMemberFnPtr(nullptr),
        mHookedFnSlot(nullptr),
        mOriginalFn()
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

        static_assert(sizeof(hookFn) == sizeof(MemFnPtr));

        mHookThisPtr = hookThisPtr;
        mHookMemberFnPtr = hookFn;

        MemFnPtr* vtable = *(MemFnPtr**)thisPtr;
        mHookedFnSlot = &vtable[vtableSlot];
        mOriginalFn = EditVTable(vtable, vtableSlot, reinterpret_cast<const MemFnPtr*>(&hookFn));
    }

    void Unhook()
    {
        if (!mHookThisPtr)
        {
            return;
        }
        mHookThisPtr = nullptr;
        EditVTable(mHookedFnSlot, 0, &mOriginalFn);
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

    MemFnPtr* mHookedFnSlot;
    MemFnPtr mOriginalFn;
};
