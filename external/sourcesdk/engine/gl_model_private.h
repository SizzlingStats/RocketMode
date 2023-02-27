
#pragma once

#include "../public/tier1/utlvector.h"
#include "../public/tier1/utlstring.h"

typedef unsigned short MDLHandle_t;
typedef void* FileNameHandle_t;
class IMaterial;
struct worldbrushdata_t;
class CEngineSprite;

enum modtype_t
{
    mod_bad = 0,
    mod_brush,
    mod_sprite,
    mod_studio
};

// only models with type "mod_brush" have this data
struct brushdata_t
{
    worldbrushdata_t* pShared;
    int firstmodelsurface, nummodelsurfaces;

    unsigned short renderHandle;
    unsigned short firstnode;
};

// only models with type "mod_sprite" have this data
struct spritedata_t
{
    int numframes;
    int width;
    int height;
    CEngineSprite* sprite;
};

struct model_t
{
    FileNameHandle_t fnHandle;
    CUtlString strName;

    int nLoadFlags; // mark loaded/not loaded
    int nServerCount; // marked at load
    IMaterial** ppMaterials; // null-terminated runtime material cache; ((intptr_t*)(ppMaterials))[-1] == nMaterials

    modtype_t type;
    int flags; // MODELFLAG_???

    // volume occupied by the model graphics	
    Vector mins, maxs;
    float radius;

    union
    {
        brushdata_t brush;
        MDLHandle_t studio;
        spritedata_t sprite;
    };
};
