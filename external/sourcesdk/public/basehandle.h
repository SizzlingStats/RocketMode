
#pragma once

#include "const.h"

class CBaseHandle
{
public:
    CBaseHandle() :
        m_Index(INVALID_EHANDLE_INDEX)
    {
    }

    void Term()
    {
        m_Index = INVALID_EHANDLE_INDEX;
    }

    bool IsValid() const
    {
        return m_Index != INVALID_EHANDLE_INDEX;
    }

    // edict index
    int GetEntryIndex() const
    {
        // There is a hack here: due to a bug in the original implementation of the 
        // entity handle system, an attempt to look up an invalid entity index in 
        // certain cirumstances might fall through to the the mask operation below.
        // This would mask an invalid index to be in fact a lookup of entity number
        // NUM_ENT_ENTRIES, so invalid ent indexes end up actually looking up the
        // last slot in the entities array. Since this slot is always empty, the 
        // lookup returns NULL and the expected behavior occurs through this unexpected
        // route.
        // A lot of code actually depends on this behavior, and the bug was only exposed
        // after a change to NUM_SERIAL_NUM_BITS increased the number of allowable
        // static props in the world. So the if-stanza below detects this case and 
        // retains the prior (bug-submarining) behavior.
        if (!IsValid())
            return NUM_ENT_ENTRIES - 1;
        return m_Index & ENT_ENTRY_MASK;
    }

    int CBaseHandle::GetSerialNumber() const
    {
        return m_Index >> NUM_SERIAL_NUM_SHIFT_BITS;
    }

    bool operator !=(const CBaseHandle& other) const
    {
        return m_Index != other.m_Index;
    }

    bool operator ==(const CBaseHandle& other) const
    {
        return m_Index == other.m_Index;
    }

protected:
    // The low NUM_SERIAL_BITS hold the index. If this value is less than MAX_EDICTS, then the entity is networkable.
	// The high NUM_SERIAL_NUM_BITS bits are the serial number.
    unsigned long m_Index;
};
