
#pragma once

class IServerGameDLL;
class ServerClass;
class SendProp;

namespace NetPropHelpers
{
    void PrintAllServerClassTables(IServerGameDLL* serverGameDll);
    void PrintServerClassTables(IServerGameDLL* serverGameDll, const char* classname);

    ServerClass* GetServerClass(IServerGameDLL* serverGameDll, const char* classname);
    SendProp* GetProp(IServerGameDLL* serverGameDll, const char* className, const char* tableName, const char* propName);
    SendProp* GetProp(ServerClass* serverClass, const char* tableName, const char* propName);
}
