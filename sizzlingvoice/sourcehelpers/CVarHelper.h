
#pragma once

class ICvar;
class ConVar;

namespace CVarHelper
{
    bool Initialize(ICvar* cvar);

    ConVar* CreateConVar(const char* name, const char* defaultValue, const char* description = "");
    void DestroyConVar(ConVar* convar);

    void UnhideAllCVars();
}
