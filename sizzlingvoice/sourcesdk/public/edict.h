
#pragma once

class IServerNetworkable;
class IServerUnknown;

class CBaseEdict
{
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
