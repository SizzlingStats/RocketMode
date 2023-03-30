
#pragma once

#include "iconvar.h"

typedef int CVarDLLIdentifier_t;

class ConCommandBase
{
public:
    virtual ~ConCommandBase() = 0;
    virtual	bool IsCommand() const = 0;
    virtual bool IsFlagSet(int flag) const = 0;
    virtual void AddFlags(int flags) = 0;
    virtual const char* GetName(void) const = 0;
    virtual const char* GetHelpText(void) const = 0;
    virtual bool IsRegistered(void) const = 0;
    virtual CVarDLLIdentifier_t	GetDLLIdentifier() const = 0;
    virtual void CreateBase(const char* pName, const char* pHelpString = 0, int flags = 0) = 0;
    virtual void Init() = 0;

    // Next ConVar in chain
	// Prior to register, it points to the next convar in the DLL.
	// Once registered, though, m_pNext is reset to point to the next
	// convar in the global list
	ConCommandBase *m_pNext;

	// Has the cvar been added to the global list?
	bool m_bRegistered;

	// Static data
	const char *m_pszName;
	const char *m_pszHelpString;
	
	// ConVar flags
	int m_nFlags;
};

class CCommand
{
public:
    int ArgC() const
    {
        return m_nArgc;
    }

    const char** ArgV() const
    {
        return m_nArgc ? (const char**)m_ppArgv : nullptr;
    }
    
    // All args that occur after the 0th arg, in string form
    const char* ArgS() const
    {
        return m_nArgv0Size ? &m_pArgSBuffer[m_nArgv0Size] : "";
    }

    // The entire command in string form, including the 0th arg
    const char* GetCommandString() const
    {
        return m_nArgc ? m_pArgSBuffer : "";
    }

    // Gets at arguments
    const char* Arg(int nIndex) const
    {
        if (nIndex < 0 || nIndex >= m_nArgc)
            return "";
        return m_ppArgv[nIndex];
    }

private:
    enum
    {
        COMMAND_MAX_ARGC = 64,
        COMMAND_MAX_LENGTH = 512,
    };

    int m_nArgc;
    int m_nArgv0Size;
    char m_pArgSBuffer[COMMAND_MAX_LENGTH];
    char m_pArgvBuffer[COMMAND_MAX_LENGTH];
    const char* m_ppArgv[COMMAND_MAX_ARGC];
};

class ConVar : public ConCommandBase, public IConVar
{
public:
    virtual ~ConVar() = 0;

    bool IsEnabled() const { return GetInt() != 0; }
    int GetInt() const { return m_pParent->m_nValue; };
    float GetFloat() const { return m_pParent->m_fValue; }

    // This either points to "this" or it points to the original declaration of a ConVar.
	// This allows ConVars to exist in separate modules, and they all use the first one to be declared.
	// m_pParent->m_pParent must equal m_pParent (ie: m_pParent must be the root, or original, ConVar).
	ConVar *m_pParent;

	// Static data
	const char* m_pszDefaultValue;
	
	// Value
	// Dynamically allocated
	char* m_pszString;
	int m_StringLength;

	// Values
	float m_fValue;
	int m_nValue;

	// Min/Max values
	bool m_bHasMin;
	float m_fMinVal;
	bool m_bHasMax;
	float m_fMaxVal;

	// Min/Max values for competitive.
	bool m_bHasCompMin;
	float m_fCompMinVal;
	bool m_bHasCompMax;
	float m_fCompMaxVal;

	bool m_bCompetitiveRestrictions;

	// Call this function when ConVar changes
	FnChangeCallback_t m_fnChangeCallback;
};
