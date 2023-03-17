
#pragma once

#include "econ_item_interface.h"
#include "econ_item.h"
#include "../../../public/networkvar.h"
#include "../../../public/tier1/utlvector.h"

struct datamap_t;
class CEconItemAttribute;
class CAttributeManager;

class CAttributeList
{
public:
    void* vtable;

    // Our list of attributes
    CUtlVector<CEconItemAttribute> m_Attributes;

    CAttributeManager* m_pManager;
};

//-----------------------------------------------------------------------------
// Purpose: An attribute that knows how to read itself from a datafile, describe itself to the user,
//			and serialize itself between Servers, Clients, and Steam.
//			Unlike the attributes created in the Game DLL, this attribute doesn't know how to actually
//			do anything in the game, it just knows how to describe itself.
//-----------------------------------------------------------------------------
class CEconItemAttribute
{
public:
    CEconItemAttribute(void* vtable, const attrib_definition_index_t iAttributeIndex, float flValue) :
        vtable(vtable),
        m_iAttributeDefinitionIndex(iAttributeIndex),
        m_flValue(flValue),
        m_nRefundableCurrency(0)
    {
    }

    CEconItemAttribute(void* vtable, const attrib_definition_index_t iAttributeIndex, uint32 unValue) :
        vtable(vtable),
        m_iAttributeDefinitionIndex(iAttributeIndex),
        m_flValue(*(float*)&unValue),
        m_nRefundableCurrency(0)
    {
    }

    bool operator==(const CEconItemAttribute& other) const
    {
        return !memcmp(this, &other, sizeof(other));
    }

    void* vtable;

    // This is the index of the attribute into the attributes read from the data files
    attrib_definition_index_t m_iAttributeDefinitionIndex;

    // This is the value of the attribute. Used to modify the item's variables.
    float m_flValue;

    // This is the value that the attribute was first set to by an item definition
    int m_nRefundableCurrency;
};

//-----------------------------------------------------------------------------
// Purpose: An item that knows how to read itself from a datafile, describe itself to the user,
//			and serialize itself between Servers, Clients, and Steam.
//
//			In the client DLL, we derive it from CDefaultClientRenderable so that
//			it can be passed in the pProxyData parameter of material proxies.
//-----------------------------------------------------------------------------
class CEconItemView : public CMaterialOverrideContainer<IEconItemInterface>
{
public:
    // Index of the item definition in the item script file.
    CNetworkVar(item_definition_index_t, m_iItemDefinitionIndex);

    // The quality of this item.
    CNetworkVar(int, m_iEntityQuality);

    // The level of this item.
    CNetworkVar(uint32, m_iEntityLevel);

    // The global index of this item, worldwide.
    itemid_t m_iItemID;
    CNetworkVar(uint32, m_iItemIDHigh);
    CNetworkVar(uint32, m_iItemIDLow);

    // Account ID of the person who has this in their inventory
    CNetworkVar(uint32, m_iAccountID);

    // Position inside the player's inventory
    CNetworkVar(uint32, m_iInventoryPosition);

    // This is an alternate source of data, if this item models something that isn't in the SO cache.
    CEconItemHandle m_pNonSOEconItem;

    bool m_bColorInit;
    bool m_bPaintOverrideInit;
    bool m_bHasPaintOverride;
    float m_flOverrideIndex;
    uint32_t m_unRGB;
    uint32_t m_unAltRGB;

    CNetworkVar(int, m_iTeamNumber);

    CNetworkVar(bool, m_bInitialized);

public:
    CNetworkVarEmbedded(CAttributeList, m_AttributeList);
    CNetworkVarEmbedded(CAttributeList, m_NetworkedDynamicAttributesForDemos);

    // Some custom gamemodes are using server plugins to modify weapon attributes.
    // This variable allows them to completely set their own attributes on a weapon
    // and have the client and server ignore the static attributes.
    CNetworkVar(bool, m_bOnlyIterateItemViewAttributes);
};
