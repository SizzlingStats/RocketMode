
#pragma once

#include "../tier1/interface.h"

class IServerEntity;
class IClientEntity;
class Vector;
class QAngle;
class CBaseEntity;
class IEntityFactoryDictionary;
class CBaseAnimating;
class CTakeDamageInfo;
class ITempEntsSystem;
class CBaseTempEntity;
class CGlobalEntityList;
class IEntityFindFilter;

class IServerTools : public IBaseInterface
{
public:
    virtual IServerEntity *GetIServerEntity( IClientEntity *pClientEntity ) = 0;
    virtual bool SnapPlayerToPosition( const Vector &org, const QAngle &ang, IClientEntity *pClientPlayer = nullptr) = 0;
    virtual bool GetPlayerPosition( Vector &org, QAngle &ang, IClientEntity *pClientPlayer = nullptr) = 0;
    virtual bool SetPlayerFOV( int fov, IClientEntity *pClientPlayer = nullptr) = 0;
    virtual int GetPlayerFOV( IClientEntity *pClientPlayer = nullptr) = 0;
    virtual bool IsInNoClipMode( IClientEntity *pClientPlayer = nullptr) = 0;

    // entity searching
    virtual CBaseEntity *FirstEntity( void ) = 0;
    virtual CBaseEntity *NextEntity( CBaseEntity *pEntity ) = 0;
    virtual CBaseEntity *FindEntityByHammerID( int iHammerID ) = 0;

    // entity query
    virtual bool GetKeyValue( CBaseEntity *pEntity, const char *szField, char *szValue, int iMaxLen ) = 0;
    virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, const char *szValue ) = 0;
    virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, float flValue ) = 0;
    virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, const Vector &vecValue ) = 0;

    // entity spawning
    virtual CBaseEntity *CreateEntityByName( const char *szClassName ) = 0;
    virtual void DispatchSpawn( CBaseEntity *pEntity ) = 0;

    // This reloads a portion or all of a particle definition file.
    // It's up to the server to decide if it cares about this file
    // Use a UtlBuffer to crack the data
    virtual void ReloadParticleDefintions( const char *pFileName, const void *pBufData, int nLen ) = 0;

    virtual void AddOriginToPVS( const Vector &org ) = 0;
    virtual void MoveEngineViewTo( const Vector &vPos, const QAngle &vAngles ) = 0;

    virtual bool DestroyEntityByHammerId( int iHammerID ) = 0;
    virtual CBaseEntity *GetBaseEntityByEntIndex( int iEntIndex ) = 0;
    virtual void RemoveEntity( CBaseEntity *pEntity ) = 0;
    virtual void RemoveEntityImmediate( CBaseEntity *pEntity ) = 0;
    virtual IEntityFactoryDictionary *GetEntityFactoryDictionary( void ) = 0;

    virtual void SetMoveType( CBaseEntity *pEntity, int val ) = 0;
    virtual void SetMoveType( CBaseEntity *pEntity, int val, int moveCollide ) = 0;
    virtual void ResetSequence( CBaseAnimating *pEntity, int nSequence ) = 0;
    virtual void ResetSequenceInfo( CBaseAnimating *pEntity ) = 0;

    virtual void ClearMultiDamage( void ) = 0;
    virtual void ApplyMultiDamage( void ) = 0;
    virtual void AddMultiDamage( const CTakeDamageInfo &pTakeDamageInfo, CBaseEntity *pEntity ) = 0;
    virtual void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore ) = 0;

    virtual ITempEntsSystem *GetTempEntsSystem( void ) = 0;
    virtual CBaseTempEntity *GetTempEntList( void ) = 0;

    virtual CGlobalEntityList *GetEntityList( void ) = 0;
    virtual bool IsEntityPtr( void *pTest ) = 0;
    virtual CBaseEntity *FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName ) = 0;
    virtual CBaseEntity *FindEntityByName( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = nullptr, CBaseEntity *pActivator = nullptr, CBaseEntity *pCaller = nullptr, IEntityFindFilter *pFilter = nullptr) = 0;
    virtual CBaseEntity *FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius ) = 0;
    virtual CBaseEntity *FindEntityByTarget( CBaseEntity *pStartEntity, const char *szName ) = 0;
    virtual CBaseEntity *FindEntityByModel( CBaseEntity *pStartEntity, const char *szModelName ) = 0;
    virtual CBaseEntity *FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = nullptr, CBaseEntity *pActivator = nullptr, CBaseEntity *pCaller = nullptr ) = 0;
    virtual CBaseEntity *FindEntityByNameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = nullptr, CBaseEntity *pActivator = nullptr, CBaseEntity *pCaller = nullptr ) = 0;
    virtual CBaseEntity *FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius ) = 0;
    virtual CBaseEntity *FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius ) = 0;
    virtual CBaseEntity *FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecMins, const Vector &vecMaxs ) = 0;
    virtual CBaseEntity *FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = nullptr, CBaseEntity *pActivator = nullptr, CBaseEntity *pCaller = nullptr ) = 0;
    virtual CBaseEntity *FindEntityGenericWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = nullptr, CBaseEntity *pActivator = nullptr, CBaseEntity *pCaller = nullptr ) = 0;
    virtual CBaseEntity *FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = nullptr, CBaseEntity *pActivator = nullptr, CBaseEntity *pCaller = nullptr ) = 0;
    virtual CBaseEntity *FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold ) = 0;
    virtual CBaseEntity *FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, char *classname ) = 0;
    virtual CBaseEntity *FindEntityProcedural( const char *szName, CBaseEntity *pSearchingEntity = nullptr, CBaseEntity *pActivator = nullptr, CBaseEntity *pCaller = nullptr ) = 0;
};

#define VSERVERTOOLS_INTERFACE_VERSION "VSERVERTOOLS003"
