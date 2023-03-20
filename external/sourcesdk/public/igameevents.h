
#pragma once

#include "tier0/platform.h"
#include "tier1/interface.h"

class bf_write;
class bf_read;

#define INTERFACEVERSION_GAMEEVENTSMANAGER2	"GAMEEVENTSMANAGER002"	// new game event manager,

class IGameEvent
{
public:
    virtual ~IGameEvent() = 0;
    virtual const char* GetName() const = 0;	// get event name

    virtual bool  IsReliable() const = 0; // if event handled reliable
    virtual bool  IsLocal() const = 0; // if event is never networked
    virtual bool  IsEmpty(const char* keyName = nullptr) = 0; // check if data field exists

    // Data access
    virtual bool  GetBool(const char* keyName = nullptr, bool defaultValue = false) = 0;
    virtual int   GetInt(const char* keyName = nullptr, int defaultValue = 0) = 0;
    virtual float GetFloat(const char* keyName = nullptr, float defaultValue = 0.0f) = 0;
    virtual const char* GetString(const char* keyName = nullptr, const char* defaultValue = "") = 0;

    virtual void SetBool(const char* keyName, bool value) = 0;
    virtual void SetInt(const char* keyName, int value) = 0;
    virtual void SetFloat(const char* keyName, float value) = 0;
    virtual void SetString(const char* keyName, const char* value) = 0;
};

class IGameEventListener2
{
public:
    DECL_DESTRUCTOR(IGameEventListener2);

    // FireEvent is called by EventManager if event just occurred
    // KeyValue memory will be freed by manager if not needed anymore
    virtual void FireGameEvent(IGameEvent* event) = 0;
};

class IGameEventManager2 : public IBaseInterface
{
public:
    virtual	~IGameEventManager2(void) = 0;

    // load game event descriptions from a file eg "resource\gameevents.res"
    virtual int LoadEventsFromFile(const char* filename) = 0;

    // removes all and anything
    virtual void  Reset() = 0;

    // adds a listener for a particular event
    virtual bool AddListener(IGameEventListener2* listener, const char* name, bool bServerSide) = 0;

    // returns true if this listener is listens to given event
    virtual bool FindListener(IGameEventListener2* listener, const char* name) = 0;

    // removes a listener 
    virtual void RemoveListener(IGameEventListener2* listener) = 0;

    // create an event by name, but doesn't fire it. returns NULL is event is not
    // known or no listener is registered for it. bForce forces the creation even if no listener is active
    virtual IGameEvent* CreateEvent(const char* name, bool bForce = false) = 0;

    // fires a server event created earlier, if bDontBroadcast is set, event is not send to clients
    virtual bool FireEvent(IGameEvent* event, bool bDontBroadcast = false) = 0;

    // fires an event for the local client only, should be used only by client code
    virtual bool FireEventClientSide(IGameEvent* event) = 0;

    // create a new copy of this event, must be free later
    virtual IGameEvent* DuplicateEvent(IGameEvent* event) = 0;

    // if an event was created but not fired for some reason, it has to bee freed, same UnserializeEvent
    virtual void FreeEvent(IGameEvent* event) = 0;

    // write/read event to/from bitbuffer
    virtual bool SerializeEvent(IGameEvent* event, bf_write* buf) = 0;
    virtual IGameEvent* UnserializeEvent(bf_read* buf) = 0; // create new KeyValues, must be deleted
};
