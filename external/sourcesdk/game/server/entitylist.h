
#pragma once

#include "../shared/entitylist_base.h"
#include "../../public/tier1/utlvector.h"

class CBaseEntity;

// Implement this class and register with gEntList to receive entity create/delete notification
class IEntityListener
{
public:
    virtual void OnEntityCreated(CBaseEntity* pEntity) = 0;
    virtual void OnEntitySpawned(CBaseEntity* pEntity) = 0;
    virtual void OnEntityDeleted(CBaseEntity* pEntity) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: a global list of all the entities in the game.  All iteration through
//			entities is done through this object.
//-----------------------------------------------------------------------------
class CGlobalEntityList : public CBaseEntityList
{
public:
    int m_iHighestEnt; // the topmost used array index
    int m_iNumEnts;
    int m_iNumEdicts;

    bool m_bClearingEntities;
    CUtlVector<IEntityListener*> m_entityListeners;
};
