
#pragma once

#include "../appframework/IAppSystem.h"
#include "../datamap.h"
#include "../tier0/basetypes.h"
#include "../tier1/utlvector.h"

typedef int ScriptDataType_t;
struct ScriptVariant_t;
class Vector;
class IScriptVM;
class CUtlBuffer;

#define VSCRIPT_INTERFACE_VERSION "VScriptManager010"

enum ScriptLanguage_t
{
    SL_NONE,
    SL_GAMEMONKEY,
    SL_SQUIRREL,
    SL_LUA,
    SL_PYTHON,

    SL_DEFAULT = SL_SQUIRREL
};

class IScriptManager : public IAppSystem
{
public:
    virtual IScriptVM* CreateVM(ScriptLanguage_t language = SL_DEFAULT) = 0;
    virtual void DestroyVM(IScriptVM* vm) = 0;
};

DECLARE_POINTER_HANDLE(HSCRIPT);
#define INVALID_HSCRIPT ((HSCRIPT)-1)

enum ExtendedFieldType
{
    FIELD_TYPEUNKNOWN = FIELD_TYPECOUNT,
    FIELD_CSTRING,
    FIELD_HSCRIPT,
    FIELD_VARIANT,
};

struct ScriptFuncDescriptor_t
{
    const char* m_pszScriptName;
    const char* m_pszFunction;
    const char* m_pszDescription;
    ScriptDataType_t m_ReturnType;
    CUtlVector<ScriptDataType_t> m_Parameters;
};

typedef bool (*ScriptBindingFunc_t)(void* pFunction, void* pContext, ScriptVariant_t* pArguments, int nArguments, ScriptVariant_t* pReturn);

struct ScriptFunctionBinding_t
{
    ScriptFuncDescriptor_t m_desc;
    ScriptBindingFunc_t m_pfnBinding;
    void* m_pFunction;
    unsigned m_flags;
};

class IScriptInstanceHelper
{
public:
    virtual void* GetProxied(void* p) = 0;
    virtual bool ToString(void* p, char* pBuf, int bufSize) = 0;
    virtual void* BindOnRead(HSCRIPT hInstance, void* pOld, const char* pszId) = 0;
};

struct ScriptClassDesc_t
{
    const char* m_pszScriptName;
    const char* m_pszClassname;
    const char* m_pszDescription;
    ScriptClassDesc_t* m_pBaseDesc;
    CUtlVector<ScriptFunctionBinding_t> m_FunctionBindings;

    void* (*m_pfnConstruct)();
    void (*m_pfnDestruct)(void*);
    IScriptInstanceHelper* pHelper; // optional helper
};

struct ScriptVariant_t
{
    ScriptVariant_t() : m_flags(0), m_type(FIELD_VOID) { m_pVector = 0; }
    ScriptVariant_t(int val) : m_flags(0), m_type(FIELD_INTEGER) { m_int = val; }
    ScriptVariant_t(float val) : m_flags(0), m_type(FIELD_FLOAT) { m_float = val; }
    ScriptVariant_t(double val) : m_flags(0), m_type(FIELD_FLOAT) { m_float = (float)val; }
    ScriptVariant_t(char val) : m_flags(0), m_type(FIELD_CHARACTER) { m_char = val; }
    ScriptVariant_t(bool val) : m_flags(0), m_type(FIELD_BOOLEAN) { m_bool = val; }
    ScriptVariant_t(HSCRIPT val) : m_flags(0), m_type(FIELD_HSCRIPT) { m_hScript = val; }
    ScriptVariant_t(const Vector& val) : m_flags(0), m_type(FIELD_VECTOR) { m_pVector = &val; }
    ScriptVariant_t(const Vector* val) : m_flags(0), m_type(FIELD_VECTOR) { m_pVector = val; }
    ScriptVariant_t(const char* val) : m_flags(0), m_type(FIELD_CSTRING) { m_pszString = val; }

    void operator=(int i) { m_type = FIELD_INTEGER; m_int = i; }
    void operator=(float f) { m_type = FIELD_FLOAT; m_float = f; }
    void operator=(double f) { m_type = FIELD_FLOAT; m_float = (float)f; }
    void operator=(const Vector& vec) { m_type = FIELD_VECTOR; m_pVector = &vec; }
    void operator=(const Vector* vec) { m_type = FIELD_VECTOR; m_pVector = vec; }
    void operator=(const char* psz) { m_type = FIELD_CSTRING; m_pszString = psz; }
    void operator=(char c) { m_type = FIELD_CHARACTER; m_char = c; }
    void operator=(bool b) { m_type = FIELD_BOOLEAN; m_bool = b; }
    void operator=(HSCRIPT h) { m_type = FIELD_HSCRIPT; m_hScript = h; }

    union
    {
        int m_int;
        float m_float;
        const char* m_pszString;
        const Vector* m_pVector;
        char m_char;
        bool m_bool;
        HSCRIPT m_hScript;
    };

    int16 m_type;
    int16 m_flags;
};

enum ScriptErrorLevel_t
{
    SCRIPT_LEVEL_WARNING = 0,
    SCRIPT_LEVEL_ERROR,
};

typedef void (*ScriptOutputFunc_t)(const char* pszText);
typedef bool (*ScriptErrorFunc_t)(ScriptErrorLevel_t eLevel, const char* pszText);

enum ScriptStatus_t
{
    SCRIPT_ERROR = -1,
    SCRIPT_DONE,
    SCRIPT_RUNNING,
};

class IScriptVM
{
public:
    virtual bool Init() = 0;
    virtual void Shutdown() = 0;

    virtual bool ConnectDebugger() = 0;
    virtual void DisconnectDebugger() = 0;

    virtual ScriptLanguage_t GetLanguage() = 0;
    virtual const char* GetLanguageName() = 0;

    virtual void AddSearchPath(const char* pszSearchPath) = 0;

    //--------------------------------------------------------

    virtual bool Frame(float simTime) = 0;

    //--------------------------------------------------------
    // Simple script usage
    //--------------------------------------------------------
    virtual ScriptStatus_t Run(const unsigned char* pszScript, bool bWait = true) = 0;

    //--------------------------------------------------------
    // Compilation
    //--------------------------------------------------------
    virtual HSCRIPT CompileScript(const unsigned char* pszScript, const char* pszId = nullptr) = 0;
    virtual void ReleaseScript(HSCRIPT) = 0;

    //--------------------------------------------------------
    // Execution of compiled
    //--------------------------------------------------------
    virtual ScriptStatus_t Run(HSCRIPT hScript, HSCRIPT hScope = nullptr, bool bWait = true) = 0;
    virtual ScriptStatus_t Run(HSCRIPT hScript, bool bWait) = 0;

    //--------------------------------------------------------
    // Scope
    //--------------------------------------------------------
    virtual HSCRIPT CreateScope(const char* pszScope, HSCRIPT hParent = nullptr) = 0;
    virtual HSCRIPT ReferenceScope(HSCRIPT hScope) = 0;
    virtual void ReleaseScope(HSCRIPT hScript) = 0;

    //--------------------------------------------------------
    // Script functions
    //--------------------------------------------------------
    virtual HSCRIPT LookupFunction(const char* pszFunction, HSCRIPT hScope = nullptr) = 0;
    virtual void ReleaseFunction(HSCRIPT hScript) = 0;

    //--------------------------------------------------------
    // Script functions (raw, use Call())
    //--------------------------------------------------------
    virtual ScriptStatus_t ExecuteFunction(HSCRIPT hFunction, ScriptVariant_t* pArgs, int nArgs, ScriptVariant_t* pReturn, HSCRIPT hScope, bool bWait) = 0;

    //--------------------------------------------------------
    // External functions
    //--------------------------------------------------------
    virtual void RegisterFunction(ScriptFunctionBinding_t* pScriptFunction) = 0;

    //--------------------------------------------------------
    // External classes
    //--------------------------------------------------------
    virtual bool RegisterClass(ScriptClassDesc_t* pClassDesc) = 0;

    //--------------------------------------------------------
    // External instances. Note class will be auto-registered.
    //--------------------------------------------------------

    virtual HSCRIPT RegisterInstance(ScriptClassDesc_t* pDesc, void* pInstance) = 0;
    virtual void SetInstanceUniqeId(HSCRIPT hInstance, const char* pszId) = 0;
    virtual void RemoveInstance(HSCRIPT) = 0;

    virtual void* GetInstanceValue(HSCRIPT hInstance, ScriptClassDesc_t* pExpectedType = nullptr) = 0;

    //----------------------------------------------------------------------------

    virtual bool GenerateUniqueKey(const char* pszRoot, char* pBuf, int nBufSize) = 0;

    //----------------------------------------------------------------------------

    virtual bool ValueExists(HSCRIPT hScope, const char* pszKey) = 0;

    virtual bool SetValue(HSCRIPT hScope, const char* pszKey, const char* pszValue) = 0;
    virtual bool SetValue(HSCRIPT hScope, const char* pszKey, const ScriptVariant_t& value) = 0;

    virtual void CreateTable(ScriptVariant_t& Table) = 0;
    virtual int	GetNumTableEntries(HSCRIPT hScope) = 0;
    virtual int GetKeyValue(HSCRIPT hScope, int nIterator, ScriptVariant_t* pKey, ScriptVariant_t* pValue) = 0;

    virtual bool GetValue(HSCRIPT hScope, const char* pszKey, ScriptVariant_t* pValue) = 0;
    virtual void ReleaseValue(ScriptVariant_t& value) = 0;

    virtual bool ClearValue(HSCRIPT hScope, const char* pszKey) = 0;

    //----------------------------------------------------------------------------

    virtual void WriteState(CUtlBuffer* pBuffer) = 0;
    virtual void ReadState(CUtlBuffer* pBuffer) = 0;
    virtual void RemoveOrphanInstances() = 0;

    virtual void DumpState() = 0;

    virtual void SetOutputCallback(ScriptOutputFunc_t pFunc) = 0;
    virtual void SetErrorCallback(ScriptErrorFunc_t pFunc) = 0;

    //----------------------------------------------------------------------------

    virtual bool RaiseException(const char* pszExceptionText) = 0;
};
