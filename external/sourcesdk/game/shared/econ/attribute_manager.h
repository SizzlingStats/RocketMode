
#pragma once

#include "econ_item_view.h"
#include "../../../public/networkvar.h"
#include "../../../public/tier1/utlvector.h"

struct datamap_t;
class CBaseEntity;
using string_t = const char*;
using EHANDLE = unsigned long; // CHandle<CBaseEntity>

// Provider types
enum attributeprovidertypes_t
{
    PROVIDER_GENERIC,
    PROVIDER_WEAPON,
};

class CAttributeManager
{
public:
    // DECLARE_DATADESC
    virtual datamap_t* GetDataDescMap() = 0;

    // DECLARE_EMBEDDED_NETWORKVAR
    virtual void NetworkStateChanged() = 0;
    virtual void NetworkStateChanged(void* pProp) = 0;

    virtual ~CAttributeManager() = 0;

    // Call this inside your entity's Spawn()
    virtual void InitializeAttributes(CBaseEntity* pEntity) = 0;

private:
    int m_nCurrentTick;
    int m_nCalls;

public:
    virtual float ApplyAttributeFloat(float flValue, CBaseEntity* pInitiator, string_t iszAttribHook = "", CUtlVector<CBaseEntity*>* pItemList = nullptr) = 0;
    virtual string_t ApplyAttributeString(string_t iszValue, CBaseEntity* pInitiator, string_t iszAttribHook = "", CUtlVector<CBaseEntity*>* pItemList = nullptr) = 0;

protected:
    CUtlVector<unsigned long> m_Providers; // entities that we receive attribute data *from*
    CUtlVector<unsigned long> m_Receivers; // entities that we provide attribute data *to*
    CNetworkVarForDerived(int, m_iReapplyProvisionParity);
    CNetworkVarForDerived(EHANDLE, m_hOuter);
    bool m_bPreventLoopback;
    CNetworkVarForDerived(attributeprovidertypes_t, m_ProviderType);
    int m_iCacheVersion; // maps to gamerules counter for global cache flushing

public:
    virtual void OnAttributeValuesChanged() = 0;

private:
    virtual float ApplyAttributeFloatWrapper(float flValue, CBaseEntity* pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*>* pItemList = nullptr) = 0;
    virtual string_t ApplyAttributeStringWrapper(string_t iszValue, CBaseEntity* pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*>* pItemList = nullptr) = 0;

    // Cached attribute results
    // We cache off requests for data, and wipe the cache whenever our providers change.
    union cached_attribute_types
    {
        float fl;
        string_t isz;
    };

    struct cached_attribute_t
    {
        string_t iAttribHook;
        cached_attribute_types in;
        cached_attribute_types out;
    };
    CUtlVector<cached_attribute_t>	m_CachedResults;
};

//-----------------------------------------------------------------------------
// Purpose: This is an attribute manager that also knows how to contain attributes.
//-----------------------------------------------------------------------------
class CAttributeContainer : public CAttributeManager
{
private:
    struct alignas(CEconItemView) CEconItemView_Hack
    {
        char buf[sizeof(CEconItemView)];
    };
    CEconItemView_Hack m_Item;
};
