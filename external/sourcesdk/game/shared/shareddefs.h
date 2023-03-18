
#pragma once

#define MAX_PLAYERS 33

// Spectator Movement modes
enum
{
    OBS_MODE_NONE = 0,	// not in spectator mode
    OBS_MODE_DEATHCAM,	// special mode for death cam animation
    OBS_MODE_FREEZECAM,	// zooms to a target, and freeze-frames on them
    OBS_MODE_FIXED,		// view from a fixed camera position
    OBS_MODE_IN_EYE,	// follow a player in first person view
    OBS_MODE_CHASE,		// follow a player in third person view
    OBS_MODE_POI,		// PASSTIME point of interest - game objective, big fight, anything interesting; added in the middle of the enum due to tons of hard-coded "<ROAMING" enum compares
    OBS_MODE_ROAMING,	// free roaming

    NUM_OBSERVER_MODES,
};

// entity flags, CBaseEntity::m_iEFlags
enum
{
    EFL_DIRTY_ABSVELOCITY = (1 << 12)
};
