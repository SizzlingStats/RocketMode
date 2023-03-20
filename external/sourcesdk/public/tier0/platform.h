
#pragma once

#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#define SINGLE_INHERITANCE __single_inheritance
#else
#define SINGLE_INHERITANCE
#define FORCEINLINE inline __attribute__((always_inline))
#endif

// This hack is to work around abstract base classes that
// declare a virtual destructor when they never should have.
// Doing that causes code bloat.
#ifdef _MSC_VER
#define DECL_DESTRUCTOR(BaseClass) \
    virtual void BaseClass##_Destructor() = 0
#define DECL_INHERITED_DESTRUCTOR(BaseClass) \
    virtual void BaseClass##_Destructor() override
#define DEFINE_INHERITED_DESTRUCTOR(DerivedClass, BaseClass) \
    void DerivedClass::BaseClass##_Destructor() {}
#else
#define DECL_DESTRUCTOR(BaseClass) \
    virtual void BaseClass##_Destructor() = 0; \
    virtual void BaseClass##_Destructor2() = 0
#define DECL_INHERITED_DESTRUCTOR(BaseClass) \
    virtual void BaseClass##_Destructor() override; \
    virtual void BaseClass##_Destructor2() override
#define DEFINE_INHERITED_DESTRUCTOR(DerivedClass, BaseClass) \
    void DerivedClass::BaseClass##_Destructor() {} \
    void DerivedClass::BaseClass##_Destructor2() {}
#endif
