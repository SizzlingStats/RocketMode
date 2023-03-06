
#pragma once

// This is the max # of players the engine can handle
#define ABSOLUTE_PLAYER_LIMIT 255  // not 256, so we can send the limit as a byte 

// How many bits to use to encode an edict.
#define	MAX_EDICT_BITS				11			// # of bits needed to represent max edicts
// Max # of edicts in a level
#define	MAX_EDICTS					(1<<MAX_EDICT_BITS)

// Used for networking ehandles.
#define NUM_ENT_ENTRY_BITS      (MAX_EDICT_BITS + 2)
#define NUM_ENT_ENTRIES         (1 << NUM_ENT_ENTRY_BITS)
#define INVALID_EHANDLE_INDEX   0xFFFFFFFF

#define NUM_SERIAL_NUM_BITS     16 // (32 - NUM_ENT_ENTRY_BITS)
#define NUM_SERIAL_NUM_SHIFT_BITS (32 - NUM_SERIAL_NUM_BITS)
#define ENT_ENTRY_MASK          (( 1 << NUM_SERIAL_NUM_BITS) - 1)

#define FL_ATCONTROLS			(1<<7) // Player can't move, but keeps key inputs for controlling another entity
