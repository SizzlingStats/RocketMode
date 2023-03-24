
#include "SizzlingVoice.h"

#include "sourcesdk/common/netmessages.h"
#include "sourcesdk/common/protocol.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/icvar.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/inetchannel.h"
#include "sourcesdk/public/iserver.h"
#include "sourcesdk/public/tier1/convar.h"
#include "sourcesdk/public/tier1/utlvector.h"

#include "HookOffsets.h"
#include <assert.h>

VTableHook<decltype(&SizzlingVoice::CLCVoiceDataProcessHook)> SizzlingVoice::sCLCVoiceDataProcessHook;

SizzlingVoice::SizzlingVoice() :
    mServer(nullptr),
    mVoiceEnable(nullptr),
    mSourceEntOverride()
{
}

bool SizzlingVoice::Init(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    IVEngineServer* engineServer = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, nullptr);
    if (engineServer)
    {
        mServer = engineServer->GetIServer();
    }

    ICvar* cvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, nullptr);

    if (!mServer || !cvar)
    {
        return false;
    }

    mVoiceEnable = cvar->FindVar("sv_voiceenable");
    if (!mVoiceEnable)
    {
        return false;
    }
    return true;
}

void SizzlingVoice::Shutdown()
{
    sCLCVoiceDataProcessHook.Unhook();
}

static const INetMessage* GetCLCVoiceData(INetChannel* channel)
{
    // excerpt from CNetChan. Used so we can get to m_NetMessages
    struct CNetChanHack
    {
        INetChannelHandler* m_MessageHandler;   // who registers and processes messages
        CUtlVector<INetMessage*> m_NetMessages; // list of registered message
    };

    INetChannelHandler* messageHandler = channel->GetMsgHandler();
    INetChannelHandler** searchPtr = reinterpret_cast<INetChannelHandler**>(channel);
    while (*searchPtr != messageHandler)
    {
        searchPtr += 1;
    }
    CNetChanHack* hack = reinterpret_cast<CNetChanHack*>(searchPtr);

    const int numMessages = hack->m_NetMessages.m_Size;
    INetMessage** messages = hack->m_NetMessages.m_pElements;
    for (int i = 0; i < numMessages; ++i)
    {
        const INetMessage* msg = messages[i];
        const int type = msg->GetType();
        if (type == clc_VoiceData)
        {
            return msg;
        }
    }
    return nullptr;
}

void SizzlingVoice::ClientActive(edict_t* pEntity)
{
    const int clientIndex = pEntity->m_EdictIndex - 1;
    IClient* client = mServer->GetClient(clientIndex);

    INetChannel* netChannel = client->GetNetChannel();
    if (!netChannel)
    {
        return;
    }

    if (!sCLCVoiceDataProcessHook.GetThisPtr())
    {
        const INetMessage* msg = GetCLCVoiceData(netChannel);
        assert(msg);

        sCLCVoiceDataProcessHook.Hook(msg, HookOffsets::ProcessVoiceData, this, &SizzlingVoice::CLCVoiceDataProcessHook);
    }
}

bool SizzlingVoice::CLCVoiceDataProcessHook()
{
    SizzlingVoice* thisPtr = sCLCVoiceDataProcessHook.GetThisPtr();
    if (!thisPtr->CLCVoiceDataProcess(reinterpret_cast<CLC_VoiceData*>(this)))
    {
        return sCLCVoiceDataProcessHook.CallOriginalFn(this);
    }
    return true;
}

bool SizzlingVoice::CLCVoiceDataProcess(CLC_VoiceData* clcVoiceData)
{
    uint8_t voiceData[4096];
    const int bitsRead = clcVoiceData->m_DataIn.ReadBitsClamped(voiceData, clcVoiceData->m_nLength);

    INetChannel* netChannel = clcVoiceData->GetNetChannel();
    IClient* client = static_cast<IClient*>(netChannel->GetMsgHandler());
    const int fromClientIndex = client->GetPlayerSlot();

    BroadcastVoiceData(fromClientIndex, voiceData, bitsRead);
    return true;
}

void SizzlingVoice::BroadcastVoiceData(int fromClientIndex, uint8_t* data, int numBits)
{
    assert(mVoiceEnable);
    if (!mVoiceEnable->IsEnabled())
    {
        return;
    }

    SVC_VoiceData svcVoiceData;
    svcVoiceData.m_nFromClient = fromClientIndex;
    svcVoiceData.m_bProximity = false;
    svcVoiceData.m_DataOut = data;

    const int clientCount = mServer->GetClientCount();
    for (int i = 0; i < clientCount; ++i)
    {
        IClient* client = mServer->GetClient(i);
        if (!client->IsActive())
        {
            continue;
        }

        const bool isSelf = (i == fromClientIndex);
        const bool bHearsPlayer = client->IsHearingClient(fromClientIndex);
        svcVoiceData.m_bProximity = client->IsProximityHearingClient(fromClientIndex);

        if (bHearsPlayer)
        {
            svcVoiceData.m_nLength = numBits;
        }
        else if (!isSelf)
        {
            continue;
        }
        else
        {
            // Still send something, just zero length (this is so the client 
            // can display something that shows the server knows it's talking).
            svcVoiceData.m_nLength = 0;
        }

        client->SendNetMsg(svcVoiceData);
    }
}
