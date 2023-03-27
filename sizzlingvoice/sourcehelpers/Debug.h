
#pragma once

namespace Debug
{
    void Initialize();

    using MsgFn = void(const char* fmt, ...);
    extern MsgFn* Msg;
}
