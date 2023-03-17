
#pragma once

#include "utlmemory.h"
#include <string.h>
#include <new>

template <class T>
inline void Destruct(T* pMemory)
{
    pMemory->~T();

#ifdef _DEBUG
    memset(reinterpret_cast<void*>(pMemory), 0xDD, sizeof(T));
#endif
}

template<typename T, class A = CUtlMemory<T>>
class CUtlVector
{
public:
    T& Element(int i);

    // Gets the base address (can change when adding elements!)
    T* Base() { return m_Memory.Base(); }
    const T* Base() const { return m_Memory.Base(); }

    // Returns the number of elements in the vector
    int Count() const { return m_Size; }

    // Is element index valid?
    bool IsValidIndex(int i) const { return (i >= 0) && (i < m_Size); }

    // Adds an element, uses copy constructor
    int AddToTail(const T& src);

    // Finds an element (element needs operator== defined)
    int Find(const T& src) const;

    // Element removal
    void FastRemove(int elem);	// doesn't preserve order
    bool FindAndFastRemove(const T& src); // removes first occurrence of src, doesn't preserve order

protected:
    // Grows the vector
    void GrowVector(int num = 1);

public:
    A m_Memory;
    int m_Size;
    T* m_pElements;

    inline void ResetDbgInfo()
    {
        m_pElements = Base();
    }
};

template< typename T, class A >
inline T& CUtlVector<T, A>::Element(int i)
{
    // Do an inline unsigned check for maximum debug-build performance.
    Assert((unsigned)i < (unsigned)m_Size);
    //StagingUtlVectorBoundsCheck(i, m_Size);
    return m_Memory[i];
}

template< typename T, class A >
inline int CUtlVector<T, A>::AddToTail(const T& src)
{
    // Can't insert something that's in the list... reallocation may hose us
    Assert((Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count())));
    
    const int index = m_Size;
    GrowVector();
    ::new(&Element(index)) T(src);
    return index;
}

//-----------------------------------------------------------------------------
// Finds an element (element needs operator== defined)
//-----------------------------------------------------------------------------
template< typename T, class A >
int CUtlVector<T, A>::Find(const T& src) const
{
    const int count = Count();
    for (int i = 0; i < count; ++i)
    {
        if (m_Memory[i] == src)
            return i;
    }
    return -1;
}

//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::FastRemove(int elem)
{
    Assert(IsValidIndex(elem));

    // Global scope to resolve conflict with Scaleform 4.0
    T& element = m_Memory[elem];
    ::Destruct(&element);
    if (m_Size > 0)
    {
        const int lastElem = m_Size - 1;
        if (elem != lastElem)
            memcpy(&element, &m_Memory[lastElem], sizeof(T));
        m_Size = lastElem;
    }
}

template< typename T, class A >
bool CUtlVector<T, A>::FindAndFastRemove(const T& src)
{
    int elem = Find(src);
    if (elem != -1)
    {
        FastRemove(elem);
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Grows the vector
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::GrowVector(int num)
{
    const int newSize = m_Size + num;
    const int numAllocated = m_Memory.NumAllocated();
    if (newSize > numAllocated)
    {
        MEM_ALLOC_CREDIT_CLASS();
        m_Memory.Grow(newSize - numAllocated);
    }
    m_Size = newSize;
    ResetDbgInfo();
}
