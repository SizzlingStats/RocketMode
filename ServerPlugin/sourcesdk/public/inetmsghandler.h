
#pragma once

class INetChannel;
typedef struct netpacket_s netpacket_t;

class INetChannelHandler
{
public:
    virtual	~INetChannelHandler() = 0;
    virtual void ConnectionStart(INetChannel* chan) = 0;	// called first time network channel is established
    virtual void ConnectionClosing(const char* reason) = 0; // network channel is being closed by remote site
    virtual void ConnectionCrashed(const char* reason) = 0; // network error occured
    virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) = 0;	// called each time a new packet arrived
    virtual void PacketEnd() = 0; // all messages has been parsed
    virtual void FileRequested(const char* fileName, unsigned int transferID) = 0; // other side request a file for download
    virtual void FileReceived(const char* fileName, unsigned int transferID) = 0; // we received a file
    virtual void FileDenied(const char* fileName, unsigned int transferID) = 0;	// a file request was denied by other side
    virtual void FileSent(const char* fileName, unsigned int transferID) = 0;	// we sent a file
};

class IConnectionlessPacketHandler
{
public:
    virtual	~IConnectionlessPacketHandler() = 0;
    virtual bool ProcessConnectionlessPacket(netpacket_t* packet) = 0;	// process a connectionless packet
};
