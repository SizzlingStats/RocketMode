
#pragma once

class IServerGameDLL;

namespace NetPropHelpers
{
    void PrintAllServerClassTables(IServerGameDLL* serverGameDll);
    void PrintServerClassTables(IServerGameDLL* serverGameDll, const char* classname);
}
