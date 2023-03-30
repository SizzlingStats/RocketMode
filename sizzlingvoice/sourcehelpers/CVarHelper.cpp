
#include "CVarHelper.h"
#include "sourcesdk/public/icvar.h"
#include "sourcesdk/public/tier1/convar.h"
#include <malloc.h>
#include <assert.h>

static ICvar* sCvar;
static ConVar* sAnyConVar;

bool CVarHelper::Initialize(ICvar* cvarInterface)
{
    sCvar = cvarInterface;
    sAnyConVar = cvarInterface->FindVar("achievement_debug");
    return sAnyConVar;
}

// Note: GetDLLIdentifier is going to return the value of
// whichever convar was used for the vtable.
static ConVar* AllocConvar(ConVar* AnyExistingConVar)
{
    assert(!AnyExistingConVar->IsCommand());

    ConVar* convar = (ConVar*)calloc(1, sizeof(ConVar));

    unsigned char** ConVarVtable = *(unsigned char***)AnyExistingConVar;
    unsigned char** IConVarVtable = *(unsigned char***)(IConVar*)AnyExistingConVar;

    *(unsigned char***)convar = ConVarVtable;
    *(unsigned char***)(IConVar*)convar = IConVarVtable;

    return convar;
}

ConVar* CVarHelper::CreateConVar(const char* name, const char* defaultValue, const char* description /*= ""*/)
{
    assert(sAnyConVar);
    assert(name);
    assert(defaultValue);

    ConVar* convar = AllocConvar(sAnyConVar);
    convar->m_pParent = convar;
    convar->m_pszDefaultValue = defaultValue;
    convar->CreateBase(name, description, 0);
    convar->SetValue(defaultValue);

    return convar;
}

void CVarHelper::DestroyConVar(ConVar* convar)
{
    if (convar)
    {
        sCvar->UnregisterConCommand(convar);
        convar->~ConVar();
        free(convar);
    }
}

void CVarHelper::UnhideAllCVars()
{
    ConCommandBase* command = sCvar->GetCommands();
    while (command)
    {
        command->m_nFlags &= ~(FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN);
        command = command->m_pNext;
    }
}
