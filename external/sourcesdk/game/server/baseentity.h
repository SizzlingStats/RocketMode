
#pragma once

#include "../../public/iserverentity.h"

class ServerClass;
struct datamap_t;

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity : public IServerEntity
{
public:
    virtual ~CBaseEntity() = 0;

    // DECLARE_SERVERCLASS();
    virtual ServerClass* GetServerClass() = 0;
    virtual int YouForgotToImplementOrDeclareServerClass() = 0;

    // DECLARE_DATADESC();
    virtual datamap_t* GetDataDescMap() = 0;

    // Don't need the rest for now.
};
