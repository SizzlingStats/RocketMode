
#pragma once

class IGameSystem
{
public:
    // GameSystems are expected to implement these methods.
    virtual char const* Name() = 0;

    // Init, shutdown
    // return true on success. false to abort DLL init!
    virtual bool Init() = 0;
    virtual void PostInit() = 0;
    virtual void Shutdown() = 0;

    // Level init, shutdown
    virtual void LevelInitPreEntity() = 0;
    // entities are created / spawned / precached here
    virtual void LevelInitPostEntity() = 0;

    virtual void LevelShutdownPreClearSteamAPIContext() = 0;
    virtual void LevelShutdownPreEntity() = 0;
    // Entities are deleted / released here...
    virtual void LevelShutdownPostEntity() = 0;
    // end of level shutdown

    // Called during game save
    virtual void OnSave() = 0;

    // Called during game restore, after the local player has connected and entities have been fully restored
    virtual void OnRestore() = 0;

    // Called every frame. It's safe to remove an igamesystem from within this callback.
    virtual void SafeRemoveIfDesired() = 0;

    virtual bool IsPerFrame() = 0;

    // destructor, cleans up automagically....
    virtual ~IGameSystem() = 0;
};

class IGameSystemPerFrame : public IGameSystem
{
public:
    // Called each frame before entities think
    virtual void FrameUpdatePreEntityThink() = 0;
    // called after entities think
    virtual void FrameUpdatePostEntityThink() = 0;
    virtual void PreClientUpdate() = 0;
};

// Quick and dirty server system for users who don't care about precise ordering
// and usually only want to implement a few of the callbacks
class CBaseGameSystem : public IGameSystem
{
private:
    // Prevent anyone derived from CBaseGameSystem from implementing these, they need
    //  to derive from CBaseGameSystemPerFrame below!!!

    // Called each frame before entities think
    virtual void FrameUpdatePreEntityThink() = 0;
    // called after entities think
    virtual void FrameUpdatePostEntityThink() = 0;
    virtual void PreClientUpdate() = 0;
};

// Quick and dirty server system for users who don't care about precise ordering
// and usually only want to implement a few of the callbacks
class CBaseGameSystemPerFrame : public IGameSystemPerFrame
{
};

// Quick and dirty server system for users who don't care about precise ordering
// and usually only want to implement a few of the callbacks
class CAutoGameSystem : public CBaseGameSystem
{
public:
    CAutoGameSystem* m_pNext;
    char const* m_pszName;
};

//-----------------------------------------------------------------------------
// Purpose: This is a CAutoGameSystem which also cares about the "per frame" hooks
//-----------------------------------------------------------------------------
class CAutoGameSystemPerFrame : public CBaseGameSystemPerFrame
{
public:
    CAutoGameSystemPerFrame* m_pNext;
    char const* m_pszName;
};
