
#pragma once

template<typename T>
struct CUtlMemory
{
    T* m_pMemory;
    int m_nAllocationCount;
    int m_nGrowSize;
};
