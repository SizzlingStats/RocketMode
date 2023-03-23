
#pragma once

#include "trace.h"
#include "cmodel.h"

class CBaseEntity;

//-----------------------------------------------------------------------------
// Purpose: A trace is returned when a box is swept through the world
// NOTE: eventually more of this class should be moved up into the base class!!
//-----------------------------------------------------------------------------
class CGameTrace : public CBaseTrace
{
public:
    float fractionleftsolid; // time we left a solid, only valid if we started in solid
    csurface_t surface; // surface hit (impact surface)

    int hitgroup; // 0 == generic, non-zero is specific body part
    short physicsbone; // physics bone hit by trace in studio

    CBaseEntity* m_pEnt;

    // NOTE: this member is overloaded.
    // If hEnt points at the world entity, then this is the static prop index.
    // Otherwise, this is the hitbox index.
    int hitbox; // box hit by trace in studio

    CGameTrace() {}

private:
    // No copy constructors allowed
    CGameTrace(const CGameTrace& vOther);
};

using trace_t = CGameTrace;
