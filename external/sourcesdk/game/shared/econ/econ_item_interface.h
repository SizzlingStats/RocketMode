
#pragma once

#include <stdint.h>

using itemid_t = uint64_t;
using style_index_t = uint8_t;
using item_definition_index_t = uint16_t;
enum eEconItemOrigin;

class IEconItemAttributeIterator;
class CEconItemPaintKitDefinition;
struct GameItemDefinition_t;
class IMaterial;

class IEconItemInterface
{
public:
    virtual ~IEconItemInterface() = 0;

    // Helpers to look for specific attribute values
    virtual CEconItemPaintKitDefinition* GetCustomPainkKitDefinition(void) const = 0;
    virtual bool GetCustomPaintKitWear(float& flWear) const = 0;

    // IEconItemInterface common implementation.
    virtual bool IsTradable() const = 0;
    virtual int  GetUntradabilityFlags() const = 0;
    virtual bool IsCommodity() const = 0;
    virtual bool IsUsableInCrafting() const = 0;
    virtual bool IsMarketable() const = 0;				// can this item be listed on the Marketplace?

    // IEconItemInterface interface.
    virtual const GameItemDefinition_t* GetItemDefinition() const = 0;

    virtual itemid_t GetID() const = 0;				// intentionally not called GetItemID to avoid stomping non-virtual GetItemID() on CEconItem
    virtual uint32_t GetAccountID() const = 0;
    virtual int32_t GetQuality() const = 0;
    virtual style_index_t GetStyle() const = 0;
    virtual uint8_t GetFlags() const = 0;
    virtual eEconItemOrigin GetOrigin() const = 0;
    virtual int GetQuantity() const = 0;
    virtual uint32_t GetItemLevel() const = 0;
    virtual bool GetInUse() const = 0;			// is this item in use somewhere in the backend? (ie., cross-game trading)

    virtual const char* GetCustomName() const = 0;		// get a user-generated name, if present, otherwise NULL; return value is UTF8
    virtual const char* GetCustomDesc() const = 0;		// get a user-generated flavor text, if present, otherwise NULL; return value is UTF8

    // IEconItemInterface attribute iteration interface. This is not meant to be used for
    // attribute lookup! This is meant for anything that requires iterating over the full
    // attribute list.
    virtual void IterateAttributes(IEconItemAttributeIterator* pIterator) const = 0;

    virtual item_definition_index_t GetItemDefIndex() const = 0;

    virtual IMaterial* GetMaterialOverride(int iTeam) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Classes that want default behavior for GetMaterialOverride, which 
// currently derive from IEconItemInterface can instead derive from 
// CMaterialOverrideContainer< IEconItemInterface > and have the details
// of material overrides hidden from them. 
//-----------------------------------------------------------------------------
template <typename TBaseClass>
class CMaterialOverrideContainer : public TBaseClass
{
};
