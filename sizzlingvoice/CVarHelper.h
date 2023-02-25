
#pragma once

class ICvar;
class ConVar;

class CVarHelper
{
public:
    bool Init(ICvar* cvarInterface);
    void Shutdown();

    ConVar* CreateConVar(const char* name, const char* defaultValue, const char* description = "");
    void DestroyConVar(ConVar* convar);

    void UnhideAllCVars();

private:
    ICvar* mCvar;
    ConVar* mAnyConVar;
};
