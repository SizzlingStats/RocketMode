
#pragma once

#include "appframework/IAppSystem.h"

typedef int CVarDLLIdentifier_t;

class ConCommandBase;
class ConVar;
class ConCommand;

class ICvar : public IAppSystem
{
public:
    // Allocate a unique DLL identifier
    virtual CVarDLLIdentifier_t AllocateDLLIdentifier() = 0;

    // Register, unregister commands
    virtual void			RegisterConCommand(ConCommandBase* pCommandBase) = 0;
    virtual void			UnregisterConCommand(ConCommandBase* pCommandBase) = 0;
    virtual void			UnregisterConCommands(CVarDLLIdentifier_t id) = 0;

    // If there is a +<varname> <value> on the command line, this returns the value.
    // Otherwise, it returns NULL.
    virtual const char* GetCommandLineValue(const char* pVariableName) = 0;

    // Try to find the cvar pointer by name
    virtual ConCommandBase* FindCommandBase(const char* name) = 0;
    virtual const ConCommandBase* FindCommandBase(const char* name) const = 0;
    virtual ConVar* FindVar(const char* var_name) = 0;
    virtual const ConVar* FindVar(const char* var_name) const = 0;
    virtual ConCommand* FindCommand(const char* name) = 0;
    virtual const ConCommand* FindCommand(const char* name) const = 0;

    // Get first ConCommandBase to allow iteration
    virtual ConCommandBase* GetCommands(void) = 0;
    virtual const ConCommandBase* GetCommands(void) const = 0;

    // TODO: add the rest of the interface when needed.
};

#define CVAR_INTERFACE_VERSION "VEngineCvar004"
