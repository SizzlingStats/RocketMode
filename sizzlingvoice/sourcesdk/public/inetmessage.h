

#pragma once

class INetChannel;
class bf_read;
class bf_write;

class INetMessage
{
public:
    virtual	~INetMessage() = 0;

    // Use these to setup who can hear whose voice.
    // Pass in client indices (which are their ent indices - 1).

    virtual void	SetNetChannel(INetChannel* netchan) = 0; // netchannel this message is from/for
    virtual void	SetReliable(bool state) = 0;	// set to true if it's a reliable message

    virtual bool	Process(void) = 0; // calles the recently set handler to process this message

    virtual	bool	ReadFromBuffer(bf_read& buffer) = 0; // returns true if parsing was OK
    virtual	bool	WriteToBuffer(bf_write& buffer) = 0;	// returns true if writing was OK

    virtual bool	IsReliable(void) const = 0;  // true, if message needs reliable handling

    virtual int				GetType(void) const = 0; // returns module specific header tag eg svc_serverinfo
    virtual int				GetGroup(void) const = 0;	// returns net message group of this message
    virtual const char* GetName(void) const = 0;	// returns network message name, eg "svc_serverinfo"
    virtual INetChannel* GetNetChannel(void) const = 0;
    virtual const char* ToString(void) const = 0; // returns a human readable string about message content

    virtual bool    BIncomingMessageForProcessing(double, int) const = 0;
    virtual int     GetSize() const = 0;
    virtual void    SetRatePolicy() = 0;
};