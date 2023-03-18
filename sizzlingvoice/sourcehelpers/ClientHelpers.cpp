
#include "ClientHelpers.h"

#include "sourcesdk/common/netmessages.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/tier1/utlvector.h"

#include <assert.h>

static int GetViewEntityOffset(IClient* anyClient, IVEngineServer* engineServer)
{
    // layout of CGameClient near m_pViewEntity.
    // Nothing sets m_pViewEntity
    struct GetViewEntityHack
    {
        // Identity information.
        edict_t* edict; // EDICT_NUM(clientnum+1)

        struct SoundInfo_t;
        CUtlVector<SoundInfo_t>	m_Sounds; // game sounds

        const edict_t* m_pViewEntity; // View Entity (camera or the client itself)
    };

    const int clientEntIndex = anyClient->GetPlayerSlot() + 1;
    edict_t* clientEdict = engineServer->PEntityOfEntIndex(clientEntIndex);
    assert(clientEdict);
    // scan for the pointer to CGameClient::m_pViewEntity.
    edict_t** edict = reinterpret_cast<edict_t**>(anyClient);
    while (*edict != clientEdict)
    {
        edict += 1;
    }
    const GetViewEntityHack* hack = reinterpret_cast<GetViewEntityHack*>(edict);
    return ((int)&hack->m_pViewEntity - (int)anyClient);
}

int ClientHelpers::sViewEntityOffset;

void ClientHelpers::InitializeOffsets(IClient* anyClient, IVEngineServer* engineServer)
{
    sViewEntityOffset = GetViewEntityOffset(anyClient, engineServer);
    assert(sViewEntityOffset > 0);
}

bool ClientHelpers::SetViewEntity(IClient* client, edict_t* viewTarget)
{
    SVC_SetView setview;
    if (viewTarget)
    {
        setview.m_nEntityIndex = viewTarget->m_EdictIndex;
    }
    else
    {
        const int entIndex = client->GetPlayerSlot() + 1;
        setview.m_nEntityIndex = entIndex;
    }

    if (client->SendNetMsg(setview, true))
    {
        edict_t** viewEntity = (edict_t**)((char*)client + sViewEntityOffset);
        *viewEntity = viewTarget;
        return true;
    }
    return false;
}
