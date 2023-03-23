
#pragma once

#include "dt_common.h"
#include "const.h"
#include "bitvec.h"

class SendTable;
class RecvProp;
class SendProp;
using ArrayLengthSendProxyFn = void*;
using SendVarProxyFn = void*;
class CSendTablePrecalc;

class CSendProxyRecipients
{
public:
    // Make sure we have enough room for the max possible player count
    CBitVec<ABSOLUTE_PLAYER_LIMIT> m_Bits;
};

typedef void* (*SendTableProxyFn)(
    const SendProp* pProp,
    const void* pStructBase,
    const void* pData,
    CSendProxyRecipients* pRecipients,
    int objectID);

class SendProp
{
public:
    virtual ~SendProp() = 0;

    SendTable* GetDataTable() const { return m_pDataTable; }
    int GetOffset() const { return m_Offset; }
    const char* GetName() const { return m_pVarName; }
    int GetFlags() const { return m_Flags; }
    bool IsExcludeProp() const { return (m_Flags & SPROP_EXCLUDE) != 0; }
    char const* GetExcludeDTName() const { return m_pExcludeDTName; }
    SendPropType GetType() const { return m_Type; }
    int GetNumElements() const { return m_nElements; }
    SendTableProxyFn GetDataTableProxyFn() const { return m_DataTableProxyFn; }

public:
    RecvProp* m_pMatchingRecvProp;	// This is temporary and only used while precalculating
    // data for the decoders.

    SendPropType m_Type;
    int m_nBits;
    float m_fLowValue;
    float m_fHighValue;

    SendProp* m_pArrayProp; // If this is an array, this is the property that defines each array element.
    ArrayLengthSendProxyFn m_ArrayLengthProxy; // This callback returns the array length.

    int m_nElements; // Number of elements in the array (or 1 if it's not an array).
    int m_ElementStride; // Pointer distance between array elements.

    const char* m_pExcludeDTName; // If this is an exclude prop, then this is the name of the datatable to exclude a prop from.
    const char* m_pParentArrayPropName;

    const char* m_pVarName;
    float m_fHighLowMul;

    int m_Flags; // SPROP_ flags.

    SendVarProxyFn m_ProxyFn; // NULL for DPT_DataTable.
    SendTableProxyFn m_DataTableProxyFn; // Valid for DPT_DataTable.

    SendTable* m_pDataTable;

    // SENDPROP_VECTORELEM makes this negative to start with so we can detect that and
    // set the SPROP_IS_VECTOR_ELEM flag.
    int m_Offset;

    // Extra data bound to this property.
    const void* m_pExtraData;
};

class SendTable
{
public:
    const char* GetName() const { return m_pNetTableName; }
    SendProp* GetProp(int i) const { return &m_pProps[i]; }
    int GetNumProps() const { return m_nProps; }
    void SetWriteFlag(bool bHasBeenWritten) { m_bHasBeenWritten = bHasBeenWritten; }
    bool GetWriteFlag() const { return m_bHasBeenWritten; }

public:
    SendProp* m_pProps;
    int m_nProps;

    const char* m_pNetTableName; // The name matched between client and server.

    // The engine hooks the SendTable here.
    CSendTablePrecalc* m_pPrecalc;

protected:
    bool m_bInitialized : 1;
    bool m_bHasBeenWritten : 1;
    bool m_bHasPropsEncodedAgainstCurrentTickCount : 1; // m_flSimulationTime and m_flAnimTime, e.g.
};
