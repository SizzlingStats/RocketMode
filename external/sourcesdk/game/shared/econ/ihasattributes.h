
#pragma once

class CAttributeManager;
class CAttributeContainer;
class CBaseEntity;
class CAttributeList;

// To allow an entity to have attributes, derive it from IHasAttributes and 
// contain an CAttributeManager in it. Then:
//		- Call InitializeAttributes() before your entity's Spawn()
//		- Call AddAttribute() to add attributes to the entity
//		- Call all the CAttributeManager hooks at the appropriate times in your entity.
// To get networking of the attributes to work on your entity:
//		- Add this to your entity's send table:
//			SendPropDataTable( SENDINFO_DT( m_AttributeManager ), &REFERENCE_SEND_TABLE(DT_AttributeManager) ),
//		- Call this inside your entity's OnDataChanged():
//			GetAttributeManager()->OnDataChanged( updateType );

//-----------------------------------------------------------------------------
// Purpose: Derive from this if your entity wants to contain attributes.
//-----------------------------------------------------------------------------
class IHasAttributes
{
public:
    virtual CAttributeManager* GetAttributeManager() = 0;
    virtual CAttributeContainer* GetAttributeContainer() = 0;
    virtual CBaseEntity* GetAttributeOwner() = 0;
    virtual CAttributeList* GetAttributeList() = 0;

    // Reapply yourself to whoever you should be providing attributes to.
    virtual void ReapplyProvision() = 0;
};
