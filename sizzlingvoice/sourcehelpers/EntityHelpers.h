
#pragma once

struct datamap_t;
class CBaseEntity;
struct edict_t;
class IVEngineServer;
class IServerGameDLL;
class ServerClass;
class SendProp;

namespace EntityHelpers
{
    // Datamaps
    int GetDatamapVarOffset(datamap_t* pDatamap, const char* szVarName);

    // Netprops
    // Call this after manually editing a net prop
    void StateChanged(edict_t* edict, unsigned short offset, IVEngineServer* engineServer);
    void FullStateChanged(edict_t* edict, IVEngineServer* engineServer);

    // SendTables
    void PrintAllServerClassTables(IServerGameDLL* serverGameDll);
    void PrintServerClassTables(IServerGameDLL* serverGameDll, const char* classname);

    ServerClass* GetServerClass(IServerGameDLL* serverGameDll, const char* classname);
    SendProp* GetProp(IServerGameDLL* serverGameDll, const char* className, const char* tableName, const char* propName);
    SendProp* GetProp(ServerClass* serverClass, const char* tableName, const char* propName);
}
