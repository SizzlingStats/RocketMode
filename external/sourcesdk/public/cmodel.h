
#pragma once

struct csurface_t
{
    const char* name;
    short surfaceProps;
    unsigned short flags; // BUGBUG: These are declared per surface, not per material, but this database is per-material now
};
