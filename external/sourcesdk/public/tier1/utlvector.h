
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
    const T& Element(int i) const;

    // Gets the base address (can change when adding elements!)
    T* Base() { return m_Memory.Base(); }
    const T* Base() const { return m_Memory.Base(); }

    // Returns the number of elements in the vector
    int Count() const { return m_Size; }

    // Is element index valid?
    bool IsValidIndex(int i) const { return (i >= 0) && (i < m_Size); }

    // Adds an element, uses copy constructor
    int AddToTail(const T& src);
    int InsertBefore(int elem, const T& src);
    int InsertAfter(int elem, const T& src);

    // Finds an element (element needs operator== defined)
    int Find(const T& src) const;

    // Element removal
    void FastRemove(int elem);	// doesn't preserve order
    bool FindAndFastRemove(const T& src); // removes first occurrence of src, doesn't preserve order
    void RemoveAll(); // doesn't deallocate memory

protected:
    // Grows the vector
    void GrowVector(int num = 1);

    // Shifts elements....
    void ShiftElementsRight(int elem, int num = 1);

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
inline const T& CUtlVector<T, A>::Element(int i) const
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
    Assert((Base() == nullptr) || (&src < Base()) || (&src >= (Base() + Count())));
    
    const int index = m_Size;
    GrowVector();
    ::new(&Element(index)) T(src);
    return index;
}

template< typename T, class A >
int CUtlVector<T, A>::InsertBefore(int elem, const T& src)
{
    // Can't insert something that's in the list... reallocation may hose us
    Assert((Base() == nullptr) || (&src < Base()) || (&src >= (Base() + Count())));

    // Can insert at the end
    Assert((elem == Count()) || IsValidIndex(elem));

    GrowVector();
    ShiftElementsRight(elem);
    ::new(&Element(elem)) T(src);
    return elem;
}

template< typename T, class A >
inline int CUtlVector<T, A>::InsertAfter(int elem, const T& src)
{
    // Can't insert something that's in the list... reallocation may hose us
    Assert((Base() == nullptr) || (&src < Base()) || (&src >= (Base() + Count())));
    return InsertBefore(elem + 1, src);
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

template< typename T, class A >
void CUtlVector<T, A>::RemoveAll()
{
    for (int i = m_Size; --i >= 0; )
    {
        // Global scope to resolve conflict with Scaleform 4.0
        ::Destruct(&Element(i));
    }
    m_Size = 0;
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

//-----------------------------------------------------------------------------
// Shifts elements
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::ShiftElementsRight(int elem, int num)
{
    Assert(IsValidIndex(elem) || (m_Size == 0) || (num == 0));
    int numToMove = m_Size - elem - num;
    if ((numToMove > 0) && (num > 0))
        memmove(&Element(elem + num), &Element(elem), numToMove * sizeof(T));
}
