
#pragma once

#include "sourcesdk/game/shared/shareddefs.h"
#include <list>

class IServer;
struct edict_t;

class RocketMode
{
public:
    RocketMode();
    ~RocketMode();

    bool Init(IServer* server);
    void Shutdown();

    void GameFrame(bool simulating);

    void ClientDisconnect(edict_t* pEntity);

    // These are called before the edict even has its vtable setup initialized.
    // Need a better way to get entity spawn callbacks.
    void OnEdictAllocated(edict_t* edict);
    void OnEdictFreed(const edict_t* edict);

private:
    IServer* mServer;

    struct State
    {
        edict_t* rocket;
    };
    State mClientStates[MAX_PLAYERS];

    std::list<edict_t*> mTrackingForBaseEnt;
};
