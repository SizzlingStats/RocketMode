
#pragma once

#include "../../public/basehandle.h"

class IHandleEntity;

class CEntInfo
{
public:
    IHandleEntity* m_pEntity;
    int m_SerialNumber;
    CEntInfo* m_pPrev;
    CEntInfo* m_pNext;
};

class CBaseEntityList
{
protected:
    // These are notifications to the derived class. It can cache info here if it wants.
    virtual void OnAddEntity(IHandleEntity* pEnt, CBaseHandle handle) = 0;

    // It is safe to delete the entity here. We won't be accessing the pointer after
    // calling OnRemoveEntity.
    virtual void OnRemoveEntity(IHandleEntity* pEnt, CBaseHandle handle) = 0;

private:

    class CEntInfoList
    {
        CEntInfo* m_pHead;
        CEntInfo* m_pTail;
    };

    // The first MAX_EDICTS entities are networkable. The rest are client-only or server-only.
    CEntInfo m_EntPtrArray[NUM_ENT_ENTRIES];
    CEntInfoList	m_activeList;
    CEntInfoList	m_freeNonNetworkableList;
};
