
#pragma once

#include "econ_item_interface.h"

class CEconItem;

using itemid_t = unsigned long long;
using CSteamID = unsigned long long;
using attrib_definition_index_t = uint16_t;

//-----------------------------------------------------------------------------
// Purpose: Maintains a handle to an CEconItem.  If the item gets deleted, this
//			handle will return NULL when dereferenced
//-----------------------------------------------------------------------------
class CEconItemHandle
{
public:
    void* vtable;

    CEconItem* m_pItem;         // The item
    itemid_t m_iItemID;         // The stored itemID
    CSteamID m_OwnerSteamID;    // Steam ID of the item owner.  Used for registering/unregistering from SOCache
};
