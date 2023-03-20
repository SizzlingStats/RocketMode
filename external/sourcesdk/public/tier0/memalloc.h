
#pragma once

class IMemAlloc
{
public:
#ifdef _WIN32
    virtual void* Alloc(size_t nSize) = 0;
    virtual void* Alloc(size_t nSize, const char* pFileName, int nLine) = 0;

    virtual void* Realloc(void* pMem, size_t nSize) = 0;
    virtual void* Realloc(void* pMem, size_t nSize, const char* pFileName, int nLine) = 0;

    virtual void Free(void* pMem) = 0;
    virtual void Free(void* pMem, const char* pFileName, int nLine) = 0;

    virtual void* Expand_NoLongerSupported(void* pMem, size_t nSize) = 0;
    virtual void* Expand_NoLongerSupported(void* pMem, size_t nSize, const char* pFileName, int nLine) = 0;

    virtual size_t GetSize(void* pMem) = 0;
#else
    virtual void* Alloc(size_t nSize) = 0;
    virtual void* Realloc(void* pMem, size_t nSize) = 0;
    virtual void Free(void* pMem) = 0;
    virtual void* Expand_NoLongerSupported(void* pMem, size_t nSize) = 0;

    virtual void* Alloc(size_t nSize, const char* pFileName, int nLine) = 0;
    virtual void* Realloc(void* pMem, size_t nSize, const char* pFileName, int nLine) = 0;
    virtual void Free(void* pMem, const char* pFileName, int nLine) = 0;
    virtual void* Expand_NoLongerSupported(void* pMem, size_t nSize, const char* pFileName, int nLine) = 0;

    virtual size_t GetSize(void* pMem) = 0;
#endif
};
