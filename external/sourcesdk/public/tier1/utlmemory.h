
#pragma once

#include "valve_support.h"
#include "../tier0/memalloc.h"

#define UTLMEMORY_TRACK_ALLOC()
#define UTLMEMORY_TRACK_FREE()
#define MEM_ALLOC_CREDIT_CLASS()

template< class T, class I = int >
class CUtlMemory
{
public:
    // element access
    T& operator[](I i);
    const T& operator[](I i) const;

    // Gets the base address (can change when adding elements!)
    T* Base() { Assert(!IsReadOnly()); return m_pMemory; }
    const T* Base() const { return m_pMemory; }

    // Size
    int NumAllocated() const { return m_nAllocationCount; }
    int Count() const { return m_nAllocationCount; }

    // Grows the memory, so that at least allocated + num elements are allocated
    void Grow(int num = 1);

    // is the memory externally allocated?
    bool IsExternallyAllocated() const { return m_nGrowSize < 0; }

    // is the memory read only?
    bool IsReadOnly() const { return (m_nGrowSize == EXTERNAL_CONST_BUFFER_MARKER); }

protected:
    enum
    {
        EXTERNAL_BUFFER_MARKER = -1,
        EXTERNAL_CONST_BUFFER_MARKER = -2,
    };

    T* m_pMemory;
    int m_nAllocationCount;
    int m_nGrowSize;
};

template< class T, class I >
inline T& CUtlMemory<T, I>::operator[](I i)
{
    // Avoid function calls in the asserts to improve debug build performance
    Assert(m_nGrowSize != EXTERNAL_CONST_BUFFER_MARKER); //Assert( !IsReadOnly() );
    Assert((uint32)i < (uint32)m_nAllocationCount);
    return m_pMemory[(uint32)i];
}

template< class T, class I >
inline const T& CUtlMemory<T, I>::operator[](I i) const
{
    // Avoid function calls in the asserts to improve debug build performance
    Assert((uint32)i < (uint32)m_nAllocationCount);
    return m_pMemory[(uint32)i];
}

//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
inline int UtlMemory_CalcNewAllocationCount(int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem)
{
    if (nGrowSize)
    {
        nAllocationCount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
    }
    else
    {
        if (!nAllocationCount)
        {
            // Compute an allocation which is at least as big as a cache line...
            nAllocationCount = (31 + nBytesItem) / nBytesItem;
        }

        while (nAllocationCount < nNewSize)
        {
            nAllocationCount *= 2;
        }
    }

    return nAllocationCount;
}

template< class T, class I >
void CUtlMemory<T, I>::Grow(int num)
{
    Assert(num > 0);

    if (IsExternallyAllocated())
    {
        // Can't grow a buffer whose memory was externally allocated 
        Assert(0);
        return;
    }

    // Make sure we have at least numallocated + num allocations.
    // Use the grow rules specified for this memory (in m_nGrowSize)
    int nAllocationRequested = m_nAllocationCount + num;

    UTLMEMORY_TRACK_FREE();

    int nNewAllocationCount = UtlMemory_CalcNewAllocationCount(m_nAllocationCount, m_nGrowSize, nAllocationRequested, sizeof(T));

    // if m_nAllocationRequested wraps index type I, recalculate
    if ((int)(I)nNewAllocationCount < nAllocationRequested)
    {
        if ((int)(I)nNewAllocationCount == 0 && (int)(I)(nNewAllocationCount - 1) >= nAllocationRequested)
        {
            --nNewAllocationCount; // deal w/ the common case of m_nAllocationCount == MAX_USHORT + 1
        }
        else
        {
            if ((int)(I)nAllocationRequested != nAllocationRequested)
            {
                // we've been asked to grow memory to a size s.t. the index type can't address the requested amount of memory
                Assert(0);
                return;
            }
            while ((int)(I)nNewAllocationCount < nAllocationRequested)
            {
                nNewAllocationCount = (nNewAllocationCount + nAllocationRequested) / 2;
            }
        }
    }

    m_nAllocationCount = nNewAllocationCount;

    UTLMEMORY_TRACK_ALLOC();

    extern IMemAlloc* g_pMemAlloc;
    if (m_pMemory)
    {
        MEM_ALLOC_CREDIT_CLASS();
        m_pMemory = (T*)g_pMemAlloc->Realloc(m_pMemory, m_nAllocationCount * sizeof(T));
        Assert(m_pMemory);
    }
    else
    {
        MEM_ALLOC_CREDIT_CLASS();
        m_pMemory = (T*)g_pMemAlloc->Alloc(m_nAllocationCount * sizeof(T));
        Assert(m_pMemory);
    }
}
