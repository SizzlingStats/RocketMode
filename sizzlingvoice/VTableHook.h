
#pragma once

template<class T> struct get_class;
template<class T, class R> struct get_class<R T::*> { using ret = R; using type = T; };

struct FnPtr
{
    unsigned int mAddr;
};

// Member function pointer representation.
// https://github.com/minlux/ptmf
// https://devblogs.microsoft.com/oldnewthing/20040209-00/?p=40713
struct MemFnPtr
{
    union
    {
#ifndef _MSC_VER
        // if mIsVirtual == 1, mVTableByteOffset is used to call from the vtable.
        // Otherwise, mFunc is used as the call/jump target.
        struct
        {
            unsigned int mIsVirtual : 1;
            unsigned int mPadding : 31;
        };
        unsigned int mVTableByteOffset;
#endif
        FnPtr mFunc;
    };
#ifndef _MSC_VER
    // this ptr byte adjustment before calling function.
    // msvc multiple inheritance and gcc single and multiple use this.
    // msvc single inheritance doesn't have this member.
    unsigned int mAdjustor;
#endif
};

template<typename R, typename... Args>
R CallVFunc(int vtableOffset, void* thisPtr, Args... args)
{
    FnPtr* vtable = *(FnPtr**)thisPtr;

    struct Dummy
    {
        union
        {
            R (Dummy::*fn)(Args...);
            MemFnPtr mfn;
        };
    };
    static_assert(sizeof(MemFnPtr) == sizeof(Dummy::fn));
    Dummy dummy;
    dummy.mfn.mFunc = vtable[vtableOffset];
#ifndef _MSC_VER
    dummy.mfn.mAdjustor = 0;
#endif
    return (reinterpret_cast<Dummy*>(thisPtr)->*dummy.fn)(args...);
}

FnPtr EditVTable(FnPtr* vtable, int slot, const FnPtr* replacementFn);

template<typename HookMemberFnPtr>
class VTableHook
{
    // To make sure a multiple inheritance hook class isn't used with msvc.
    static_assert(sizeof(HookMemberFnPtr) == sizeof(MemFnPtr));

    using HookClassType = typename get_class<HookMemberFnPtr>::type;

public:
    VTableHook() :
        mHookThisPtr(nullptr),
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

        mHookThisPtr = hookThisPtr;

        FnPtr* vtable = *(FnPtr**)thisPtr;
        mHookedFnSlot = &vtable[vtableSlot];
        mOriginalFn.mFunc = EditVTable(vtable, vtableSlot, &reinterpret_cast<const MemFnPtr&>(hookFn).mFunc);
#ifndef _MSC_VER
        mOriginalFn.mAdjustor = 0;
#endif
    }

    void Unhook()
    {
        if (!mHookThisPtr)
        {
            return;
        }
        mHookThisPtr = nullptr;
        EditVTable(mHookedFnSlot, 0, &mOriginalFn.mFunc);
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
    FnPtr* mHookedFnSlot;

    MemFnPtr mOriginalFn;
};
