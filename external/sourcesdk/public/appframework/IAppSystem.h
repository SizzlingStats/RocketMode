
#pragma once

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);

enum InitReturnVal_t;

class IAppSystem
{
public:
    // Here's where the app systems get to learn about each other 
    virtual bool Connect(CreateInterfaceFn factory) = 0;
    virtual void Disconnect() = 0;

    // Here's where systems can access other interfaces implemented by this object
    // Returns NULL if it doesn't implement the requested interface
    virtual void* QueryInterface(const char* pInterfaceName) = 0;

    // Init, shutdown
    virtual InitReturnVal_t Init() = 0;
    virtual void Shutdown() = 0;
};
