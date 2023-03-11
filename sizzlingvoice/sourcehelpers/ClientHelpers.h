
#pragma once

class IClient;
class IVEngineServer;
struct edict_t;

namespace ClientHelpers
{
    void InitializeOffsets(IClient* anyClient, IVEngineServer* engineServer);

    bool SetViewEntity(IClient* client, edict_t* viewTarget);
}
