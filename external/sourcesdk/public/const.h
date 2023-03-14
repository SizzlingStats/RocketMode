
#pragma once

// This is the max # of players the engine can handle
#define ABSOLUTE_PLAYER_LIMIT 255  // not 256, so we can send the limit as a byte 

// How many bits to use to encode an edict.
#define	MAX_EDICT_BITS				11			// # of bits needed to represent max edicts
// Max # of edicts in a level
#define	MAX_EDICTS					(1<<MAX_EDICT_BITS)

// Used for networking ehandles.
#ifdef SDK_COMPAT
#define NUM_ENT_ENTRY_BITS      (MAX_EDICT_BITS + 1)
#else
#define NUM_ENT_ENTRY_BITS      (MAX_EDICT_BITS + 2)
#endif
#define NUM_ENT_ENTRIES         (1 << NUM_ENT_ENTRY_BITS)
#define INVALID_EHANDLE_INDEX   0xFFFFFFFF

#ifdef SDK_COMPAT
#define NUM_SERIAL_NUM_BITS     (32 - NUM_ENT_ENTRY_BITS)
#else
#define NUM_SERIAL_NUM_BITS     16 // (32 - NUM_ENT_ENTRY_BITS)
#endif
#define NUM_SERIAL_NUM_SHIFT_BITS (32 - NUM_SERIAL_NUM_BITS)
#ifdef SDK_COMPAT
#define ENT_ENTRY_MASK			(NUM_ENT_ENTRIES - 1)
#else
#define ENT_ENTRY_MASK          (( 1 << NUM_SERIAL_NUM_BITS) - 1)
#endif

#define	FL_ONGROUND				(1<<0)	// At rest / on the ground
#define FL_DUCKING				(1<<1)	// Player flag -- Player is fully crouched
#define FL_ANIMDUCKING			(1<<2)	// Player flag -- Player is in the process of crouching or uncrouching but could be in transition
// examples:                                   Fully ducked:  FL_DUCKING &  FL_ANIMDUCKING
//           Previously fully ducked, unducking in progress:  FL_DUCKING & !FL_ANIMDUCKING
//                                           Fully unducked: !FL_DUCKING & !FL_ANIMDUCKING
//           Previously fully unducked, ducking in progress: !FL_DUCKING &  FL_ANIMDUCKING
#define	FL_WATERJUMP			(1<<3)	// player jumping out of water
#define FL_ONTRAIN				(1<<4) // Player is _controlling_ a train, so movement commands should be ignored on client during prediction.
#define FL_INRAIN				(1<<5)	// Indicates the entity is standing in rain
#define FL_FROZEN				(1<<6) // Player is frozen for 3rd person camera
#define FL_ATCONTROLS			(1<<7) // Player can't move, but keeps key inputs for controlling another entity
#define	FL_CLIENT				(1<<8)	// Is a player
#define FL_FAKECLIENT			(1<<9)	// Fake client, simulated server side; don't send network messages to them
// NON-PLAYER SPECIFIC (i.e., not used by GameMovement or the client .dll ) -- Can still be applied to players, though
#define	FL_INWATER				(1<<10)	// In water

// NOTE if you move things up, make sure to change this value
#define PLAYER_FLAG_BITS		11

#define	FL_FLY					(1<<11)	// Changes the SV_Movestep() behavior to not need to be on ground
#define	FL_SWIM					(1<<12)	// Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
#define	FL_CONVEYOR				(1<<13)
#define	FL_NPC					(1<<14)
#define	FL_GODMODE				(1<<15)
#define	FL_NOTARGET				(1<<16)
#define	FL_AIMTARGET			(1<<17)	// set if the crosshair needs to aim onto the entity
#define	FL_PARTIALGROUND		(1<<18)	// not all corners are valid
#define FL_STATICPROP			(1<<19)	// Eetsa static prop!		
#define FL_GRAPHED				(1<<20) // worldgraph has this ent listed as something that blocks a connection
#define FL_GRENADE				(1<<21)
#define FL_STEPMOVEMENT			(1<<22)	// Changes the SV_Movestep() behavior to not do any processing
#define FL_DONTTOUCH			(1<<23)	// Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set
#define FL_BASEVELOCITY			(1<<24)	// Base velocity has been applied this frame (used to convert base velocity into momentum)
#define FL_WORLDBRUSH			(1<<25)	// Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
#define FL_OBJECT				(1<<26) // Terrible name. This is an object that NPCs should see. Missiles, for example.
#define FL_KILLME				(1<<27)	// This entity is marked for death -- will be freed by game DLL
#define FL_ONFIRE				(1<<28)	// You know...
#define FL_DISSOLVING			(1<<29) // We're dissolving!
#define FL_TRANSRAGDOLL			(1<<30) // In the process of turning into a client side ragdoll.
#define FL_UNBLOCKABLE_BY_PLAYER (1<<31) // pusher that can't be blocked by the player

// edict->movetype values
enum MoveType_t
{
    MOVETYPE_NONE = 0,	// never moves
    MOVETYPE_ISOMETRIC,			// For players -- in TF2 commander view, etc.
    MOVETYPE_WALK,				// Player only - moving on the ground
    MOVETYPE_STEP,				// gravity, special edge handling -- monsters use this
    MOVETYPE_FLY,				// No gravity, but still collides with stuff
    MOVETYPE_FLYGRAVITY,		// flies through the air + is affected by gravity
    MOVETYPE_VPHYSICS,			// uses VPHYSICS for simulation
    MOVETYPE_PUSH,				// no clip to world, push and crush
    MOVETYPE_NOCLIP,			// No gravity, no collisions, still do velocity/avelocity
    MOVETYPE_LADDER,			// Used by players only when going onto a ladder
    MOVETYPE_OBSERVER,			// Observer movement, depends on player's observer mode
    MOVETYPE_CUSTOM,			// Allows the entity to describe its own physics

    // should always be defined as the last item in the list
    MOVETYPE_LAST = MOVETYPE_CUSTOM,

    MOVETYPE_MAX_BITS = 4
};

// edict->movecollide values
enum MoveCollide_t
{
    MOVECOLLIDE_DEFAULT = 0,

    // These ones only work for MOVETYPE_FLY + MOVETYPE_FLYGRAVITY
    MOVECOLLIDE_FLY_BOUNCE,	// bounces, reflects, based on elasticity of surface and object - applies friction (adjust velocity)
    MOVECOLLIDE_FLY_CUSTOM,	// Touch() will modify the velocity however it likes
    MOVECOLLIDE_FLY_SLIDE,  // slides along surfaces (no bounce) - applies friciton (adjusts velocity)

    MOVECOLLIDE_COUNT,		// Number of different movecollides

    // When adding new movecollide types, make sure this is correct
    MOVECOLLIDE_MAX_BITS = 3
};
