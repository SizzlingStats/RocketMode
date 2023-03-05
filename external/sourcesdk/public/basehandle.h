
#pragma once

#include "const.h"

class CBaseHandle
{
public:
    bool IsValid() const
    {
        return m_Index != INVALID_EHANDLE_INDEX;
    }

    // edict index
    int GetEntryIndex() const
    {
        return m_Index & ENT_ENTRY_MASK;
    }

protected:
    // The low NUM_SERIAL_BITS hold the index. If this value is less than MAX_EDICTS, then the entity is networkable.
	// The high NUM_SERIAL_NUM_BITS bits are the serial number.
    unsigned long m_Index;
};
