
#include "RecipientFilter.h"
#include "sourcesdk/public/iclient.h"
#include "sourcesdk/public/iserver.h"
#include <assert.h>

void RecipientFilter::IRecipientFilter_Destructor()
{
}

bool RecipientFilter::IsReliable() const
{
    return true;
}

bool RecipientFilter::IsInitMessage() const
{
    return false;
}

int RecipientFilter::GetRecipientCount() const
{
    return mRecipientCount;
}

int RecipientFilter::GetRecipientIndex(int slot) const
{
    if (slot >= 0 && slot < mRecipientCount)
    {
        return mRecipients[slot];
    }
    return -1;
}

void RecipientFilter::AddAllPlayers(IServer* server)
{
    int clientCount = server->GetClientCount();
    assert(clientCount < sNumRecipientSlots);
    if (clientCount > sNumRecipientSlots)
    {
        clientCount = sNumRecipientSlots;
    }

    int index = 0;
    for (int i = 0; i < clientCount; ++i)
    {
        IClient* client = server->GetClient(i);
        if (client->IsActive())
        {
            mRecipients[index++] = i + 1;
        }
    }
    mRecipientCount = index;
}

void RecipientFilter::SetSingleRecipient(int slot)
{
    assert(slot <= 255);

    mRecipientCount = 1;
    mRecipients[0] = slot;
}
