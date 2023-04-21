
#pragma once

#include "sourcesdk/game/shared/shareddefs.h"
#include "sourcesdk/public/irecipientfilter.h"
#include <stdint.h>

class IServer;

class RecipientFilter : public IRecipientFilter
{
    static constexpr int sNumRecipientSlots = MAX_PLAYERS + 1;

public:
    DECL_INHERITED_DESTRUCTOR(IRecipientFilter);

    virtual bool IsReliable() const override;
    virtual bool IsInitMessage() const override;

    virtual int GetRecipientCount() const override;
    virtual int GetRecipientIndex(int slot) const override;

    void AddAllPlayers(IServer* server);
    void SetSingleRecipient(int slot);

private:
    uint8_t mRecipientCount = 0;
    uint8_t mRecipients[sNumRecipientSlots];
};
