
#include "RocketMode.h"
#include "sourcesdk/common/netmessages.h"
#include "sourcesdk/public/basehandle.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/iservernetworkable.h"
#include "sourcesdk/public/iserverunknown.h"
#include "sourcehelpers/DatamapHelpers.h"

RocketMode::RocketMode() :
    mServer(nullptr),
    mClientStates(),
    mTrackingForBaseEnt()
{
}

RocketMode::~RocketMode()
{
}

bool RocketMode::Init(IServer* server)
{
    mServer = server;
    return true;
}

void RocketMode::Shutdown()
{
}

void RocketMode::GameFrame(bool simulating)
{
    for (edict_t* edict : mTrackingForBaseEnt)
    {
        if (IServerUnknown* unk = edict->m_pUnk)
        {
            CBaseEntity* ent = unk->GetBaseEntity();
            if (ent && edict->m_pNetworkable)
            {
                const char* classname = edict->m_pNetworkable->GetClassName();
                if (!strcmp(classname, "tf_projectile_rocket"))
                {
                    SVC_SetView setview;
                    setview.m_nEntityIndex = edict->m_EdictIndex;

                    static const int offset = DatamapHelpers::GetDatamapVarOffsetFromEnt(ent, "m_hOwnerEntity");
                    if (offset > 0)
                    {
                        CBaseHandle ownerEnt = *(CBaseHandle*)((char*)ent + offset);
                        if (ownerEnt.IsValid())
                        {
                            const int owningClientIndex = ownerEnt.GetEntryIndex() - 1;
                            assert(owningClientIndex < MAX_PLAYERS);
                            IClient* client = mServer->GetClient(owningClientIndex);
                            if (client && !client->IsFakeClient())
                            {
                                if (client->SendNetMsg(setview, true))
                                {
                                    mClientStates[owningClientIndex].rocket = edict;
                                }

                                // TODO: hook g_CommentarySystem.PrePlayerRunCommand as a pre usercmd process to clear weapon flags.
                                // TODO: set flags to FL_ATCONTROLS so buttons are still passed through
                                // TODO: hook g_pGameMovement->ProcessMovement for usercmds to get buttons

                                //mServerTools->GetKeyValue()
                            }
                        }
                    }
                }
            }
        }
    }
    mTrackingForBaseEnt.remove_if([this](edict_t* edict) -> bool
        {
            if (IServerUnknown* unk = edict->m_pUnk)
            {
                return !!unk->GetBaseEntity();
            }
            return false;
        });
}

void RocketMode::ClientDisconnect(edict_t* pEntity)
{
    const int clientIndex = pEntity->m_EdictIndex - 1;
    assert(clientIndex < MAX_PLAYERS);
    mClientStates[clientIndex].rocket = nullptr;
}

void RocketMode::OnEdictAllocated(edict_t* edict)
{
    mTrackingForBaseEnt.emplace_back(edict);
}

void RocketMode::OnEdictFreed(const edict_t* edict)
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        State& state = mClientStates[i];
        if (edict == state.rocket)
        {
            state.rocket = nullptr;

            SVC_SetView setview;
            setview.m_nEntityIndex = i + 1;

            IClient* client = mServer->GetClient(i);
            if (client)
            {
                client->SendNetMsg(setview, true);
            }
        }
    }
    mTrackingForBaseEnt.remove(const_cast<edict_t*>(edict));
}
