
#pragma once

#include "utlmemory.h"

template<typename T, class A = CUtlMemory<T>>
struct CUtlVector
{
    A m_Memory;
    int m_Size;
    T* m_pElements;
};
