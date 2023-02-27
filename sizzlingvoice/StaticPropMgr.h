
#pragma once

#include "sourcesdk/engine/gl_model_private.h"
#include "sourcesdk/public/engine/IStaticPropMgr.h"
#include "sourcesdk/public/tier1/utlvector.h"
#include "sourcesdk/public/mathlib/mathlib.h"
#include "sourcesdk/public/mathlib/vector.h"
#include "sourcesdk/public/basehandle.h"

typedef unsigned short SpatialPartitionHandle_t;
typedef unsigned short ModelInstanceHandle_t;
typedef unsigned short ClientRenderHandle_t;

class CStaticProp
{
    void* vtable1;
    void* vtable2;
    void* vtable3;
public:
    Vector					m_Origin;
    QAngle					m_Angles;
    model_t* m_pModel;
    SpatialPartitionHandle_t	m_Partition;
    ModelInstanceHandle_t	m_ModelInstance;
    unsigned char			m_Alpha;
    unsigned char			m_nSolidType;
    unsigned char			m_Skin;
    unsigned char			m_Flags;
    unsigned short			m_FirstLeaf;
    unsigned short			m_LeafCount;
    CBaseHandle				m_EntHandle;	// FIXME: Do I need client + server handles?
    ClientRenderHandle_t	m_RenderHandle;
    unsigned short			m_FadeIndex;	// Index into the m_StaticPropFade dictionary
    float					m_flForcedFadeScale;

    // bbox is the same for both GetBounds and GetRenderBounds since static props never move.
    // GetRenderBounds is interpolated data, and GetBounds is last networked.
    Vector					m_RenderBBoxMin;
    Vector					m_RenderBBoxMax;
    matrix3x4_t				m_ModelToWorld;
    float					m_flRadius;

    Vector					m_WorldRenderBBoxMin;
    Vector					m_WorldRenderBBoxMax;

    // FIXME: This sucks. Need to store the lighting origin off
    // because the time at which the static props are unserialized
    // doesn't necessarily match the time at which we can initialize the light cache
    Vector					m_LightingOrigin;
};

class CStaticPropMgr : public IStaticPropMgrServer
{
public:
    struct StaticPropDict_t
    {
        model_t* m_pModel;
        MDLHandle_t m_hMDL;
    };

    CUtlVector<StaticPropDict_t> m_StaticPropDict;
    CUtlVector<CStaticProp> m_StaticProps;
};
