
#include "VStdlibRandom.h"
#include "base/platform.h"
#include <assert.h>

VStdlibRandom::RandomIntFn* VStdlibRandom::RandomInt;

#ifdef _WIN32
#define VSTDLIB_MODULE "vstdlib.dll"
#else
#define VSTDLIB_MODULE "libvstdlib_srv.so"
#endif

void VStdlibRandom::Initialize()
{
    Platform::HModule hVStdlib = Platform::GetModuleHandle(VSTDLIB_MODULE);
    assert(hVStdlib);

    VStdlibRandom::RandomInt = (RandomIntFn*)Platform::GetProcAddress(hVStdlib, "RandomInt");
    assert(VStdlibRandom::RandomInt);
}
