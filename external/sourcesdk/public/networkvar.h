
#pragma once

#define CNetworkVar( type, name ) type name

#define CNetworkVarForDerived( type, name ) \
    virtual void NetworkStateChanged_##name() = 0; \
    virtual void NetworkStateChanged_##name( void *pVar ) = 0; \
    type name

#define CNetworkVarEmbedded( type, name ) \
    class NetworkVar_##name; \
    class NetworkVar_##name : public type \
    { \
    }; \
    NetworkVar_##name name; 

