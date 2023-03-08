
#pragma once

struct edict_t;
class IVEngineServer;

namespace EdictChangeHelpers
{
    void FullStateChanged(edict_t* edict, IVEngineServer* engineServer);
    // Call this after manually editing a net prop
    void StateChanged(edict_t* edict, unsigned short offset, IVEngineServer* engineServer);
}
