
#pragma once

#include <assert.h>

class IClient;
class IVEngineServer;
struct edict_t;

namespace ClientHelpers
{
    extern int sViewEntityOffset;   // m_pViewEntity

    void InitializeOffsets(IClient* anyClient, IVEngineServer* engineServer);

    // m_pViewEntity
    bool SetViewEntity(IClient* client, edict_t* viewTarget);
    
    // m_pViewEntity
    inline edict_t* GetViewEntity(IClient* client)
    {
        assert(sViewEntityOffset > 0);
        return *(edict_t**)((char*)client + sViewEntityOffset);
    }
}
