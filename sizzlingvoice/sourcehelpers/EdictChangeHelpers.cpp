
#include "EdictChangeHelpers.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"

void EdictChangeHelpers::StateChanged(edict_t* edict, unsigned short offset, IVEngineServer* engineServer)
{
    if (edict->m_fStateFlags & FL_FULL_EDICT_CHANGED)
        return;

    edict->m_fStateFlags |= FL_EDICT_CHANGED;

    IChangeInfoAccessor* accessor = engineServer->GetChangeAccessor(edict);
    CSharedEdictChangeInfo* sharedChangeInfo = engineServer->GetSharedEdictChangeInfo();

    if (accessor->GetChangeInfoSerialNumber() == sharedChangeInfo->m_iSerialNumber)
    {
        // Ok, I still own this one.
        CEdictChangeInfo* p = &sharedChangeInfo->m_ChangeInfos[accessor->GetChangeInfo()];

        // Now add this offset to our list of changed variables.		
        for (unsigned short i = 0; i < p->m_nChangeOffsets; i++)
            if (p->m_ChangeOffsets[i] == offset)
                return;

        if (p->m_nChangeOffsets == MAX_CHANGE_OFFSETS)
        {
            // Invalidate our change info.
            accessor->SetChangeInfoSerialNumber(0);
            edict->m_fStateFlags |= FL_FULL_EDICT_CHANGED; // So we don't get in here again.
        }
        else
        {
            p->m_ChangeOffsets[p->m_nChangeOffsets++] = offset;
        }
    }
    else
    {
        if (sharedChangeInfo->m_nChangeInfos == MAX_EDICT_CHANGE_INFOS)
        {
            // Shucks.. have to mark the edict as fully changed because we don't have room to remember this change.
            accessor->SetChangeInfoSerialNumber(0);
            edict->m_fStateFlags |= FL_FULL_EDICT_CHANGED;
        }
        else
        {
            // Get a new CEdictChangeInfo and fill it out.
            accessor->SetChangeInfo(sharedChangeInfo->m_nChangeInfos);
            sharedChangeInfo->m_nChangeInfos++;

            accessor->SetChangeInfoSerialNumber(sharedChangeInfo->m_iSerialNumber);

            CEdictChangeInfo* p = &sharedChangeInfo->m_ChangeInfos[accessor->GetChangeInfo()];
            p->m_ChangeOffsets[0] = offset;
            p->m_nChangeOffsets = 1;
        }
    }
}
