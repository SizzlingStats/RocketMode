
#pragma once

#include "utlmemory.h"

template<typename T, class A = CUtlMemory<T>>
class CUtlVector
{
public:
    A m_Memory;
    int m_Size;
    T* m_pElements;
};
