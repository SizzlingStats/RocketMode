
#pragma once

class IConVar;

#define FCVAR_DEVELOPMENTONLY	(1<<1)
#define FCVAR_HIDDEN			(1<<4)

//-----------------------------------------------------------------------------
// Called when a ConVar changes value
// NOTE: For FCVAR_NEVER_AS_STRING ConVars, pOldValue == NULL
//-----------------------------------------------------------------------------
typedef void (*FnChangeCallback_t)(IConVar* var, const char* pOldValue, float flOldValue);

class IConVar
{
public:
    // Value set
    virtual void SetValue(const char* pValue) = 0;
    virtual void SetValue(float flValue) = 0;
    virtual void SetValue(int nValue) = 0;

    // Return name of command
    virtual const char* GetName(void) const = 0;

    // Accessors.. not as efficient as using GetState()/GetInfo()
    // if you call these methods multiple times on the same IConVar
    virtual bool IsFlagSet(int nFlag) const = 0;
};
