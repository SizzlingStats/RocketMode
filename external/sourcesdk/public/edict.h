
#pragma once

#include "globalvars_base.h"
#include "string_t.h"

enum MapLoadType_t
{
    MapLoad_NewGame = 0,
    MapLoad_LoadGame,
    MapLoad_Transition,
    MapLoad_Background,
};

class CGlobalVars : public CGlobalVarsBase
{
    // Current map
    string_t		mapname;
    int				mapversion;
    string_t		startspot;
    MapLoadType_t	eLoadType;		// How the current map was loaded
    bool			bMapLoadFailed;	// Map has failed to load, we need to kick back to the main menu

    // game specific flags
    bool			deathmatch;
    bool			coop;
    bool			teamplay;
    // current maxentities
    int				maxEntities;

    int				serverCount;
};

class IServerNetworkable;
class IServerUnknown;

#define FL_EDICT_CHANGED	(1<<0)	// Game DLL sets this when the entity state changes
									// Mutually exclusive with FL_EDICT_PARTIAL_CHANGE.
									
#define FL_EDICT_FREE		(1<<1)	// this edict if free for reuse
#define FL_EDICT_FULL		(1<<2)	// this is a full server entity

#define FL_EDICT_FULLCHECK	(0<<0)  // call ShouldTransmit() each time, this is a fake flag
#define FL_EDICT_ALWAYS		(1<<3)	// always transmit this entity
#define FL_EDICT_DONTSEND	(1<<4)	// don't transmit this entity
#define FL_EDICT_PVSCHECK	(1<<5)	// always transmit entity, but cull against PVS

// Used by local network backdoor.
#define FL_EDICT_PENDING_DORMANT_CHECK	(1<<6)

// This is always set at the same time EFL_DIRTY_PVS_INFORMATION is set, but it 
// gets cleared in a different place.
#define FL_EDICT_DIRTY_PVS_INFORMATION	(1<<7)

// This is used internally to edict_t to remember that it's carrying a 
// "full change list" - all its properties might have changed their value.
#define FL_FULL_EDICT_CHANGED			(1<<8)


// Max # of variable changes we'll track in an entity before we treat it
// like they all changed.
#define MAX_CHANGE_OFFSETS	19
#define MAX_EDICT_CHANGE_INFOS	100

class CEdictChangeInfo
{
public:
    // Edicts remember the offsets of properties that change 
    unsigned short m_ChangeOffsets[MAX_CHANGE_OFFSETS];
    unsigned short m_nChangeOffsets;
};

// Shared between engine and game DLL.
class CSharedEdictChangeInfo
{
public:
    // Matched against edict_t::m_iChangeInfoSerialNumber to determine if its
    // change info is valid.
    unsigned short m_iSerialNumber;

    CEdictChangeInfo m_ChangeInfos[MAX_EDICT_CHANGE_INFOS];
    unsigned short m_nChangeInfos;	// How many are in use this frame.
};

class IChangeInfoAccessor
{
public:
    inline void SetChangeInfo(unsigned short info)
    {
        m_iChangeInfo = info;
    }

    inline void SetChangeInfoSerialNumber(unsigned short sn)
    {
        m_iChangeInfoSerialNumber = sn;
    }

    inline unsigned short GetChangeInfo() const
    {
        return m_iChangeInfo;
    }

    inline unsigned short GetChangeInfoSerialNumber() const
    {
        return m_iChangeInfoSerialNumber;
    }

private:
    unsigned short m_iChangeInfo;
    unsigned short m_iChangeInfoSerialNumber;
};

class CBaseEdict
{
public:
    bool IsFree() const;

public:
    int	m_fStateFlags;

    // NOTE: this is in the edict instead of being accessed by a virtual because the engine needs fast access to it.
    // int m_NetworkSerialNumber;

    // NOTE: m_EdictIndex is an optimization since computing the edict index
    // from a CBaseEdict* pointer otherwise requires divide-by-20. values for
    // m_NetworkSerialNumber all fit within a 16-bit integer range, so we're
    // repurposing the other 16 bits to cache off the index without changing
    // the overall layout or size of this struct. existing mods compiled with
    // a full 32-bit serial number field should still work. henryg 8/17/2011
    short m_NetworkSerialNumber;
    short m_EdictIndex;

    // NOTE: this is in the edict instead of being accessed by a virtual because the engine needs fast access to it.
    IServerNetworkable* m_pNetworkable;

    IServerUnknown* m_pUnk;
};

struct edict_t : public CBaseEdict
{
public:
    // The server timestampe at which the edict was freed (so we can try to use other edicts before reallocating this one)
    float		freetime;
};

inline bool CBaseEdict::IsFree() const
{
    return (m_fStateFlags & FL_EDICT_FREE) != 0;
}
