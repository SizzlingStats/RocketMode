
#pragma once

template<typename T>
class CUtlMemory
{
public:
    T* m_pMemory;
    int m_nAllocationCount;
    int m_nGrowSize;
};
