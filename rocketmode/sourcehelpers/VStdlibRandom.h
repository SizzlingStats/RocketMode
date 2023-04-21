
#pragma once

namespace VStdlibRandom
{
    void Initialize();

    // Uniform random int
    using RandomIntFn = int(int min, int max);
    extern RandomIntFn* RandomInt;
}
