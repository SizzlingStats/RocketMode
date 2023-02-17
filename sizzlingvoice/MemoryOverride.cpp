
#define MEMORY_OVERRIDE 1
#if MEMORY_OVERRIDE

#include <malloc.h>
#include <new>

void* operator new(std::size_t count)
{
    return malloc(count);
}

void* operator new[](std::size_t count)
{
    return malloc(count);
}

void* operator new(std::size_t count, std::align_val_t align)
{
    return _aligned_malloc(count, static_cast<std::size_t>(align));
}

void* operator new[](std::size_t count, std::align_val_t align)
{
    return _aligned_malloc(count, static_cast<std::size_t>(align));
}

void* operator new(std::size_t count, const std::nothrow_t&) noexcept
{
    return malloc(count);
}

void* operator new[](std::size_t count, const std::nothrow_t&) noexcept
{
    return malloc(count);
}

void* operator new(std::size_t count, std::align_val_t align, const std::nothrow_t&) noexcept
{
    return _aligned_malloc(count, static_cast<std::size_t>(align));
}

void* operator new[](std::size_t count, std::align_val_t align, const std::nothrow_t&) noexcept
{
    return _aligned_malloc(count, static_cast<std::size_t>(align));
}

void operator delete(void* ptr) noexcept
{
    free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    free(ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept
{
    _aligned_free(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept
{
    _aligned_free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept
{
    free(ptr);
}

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept
{
    _aligned_free(ptr);
}

void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept
{
    _aligned_free(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    free(ptr);
}

void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
    _aligned_free(ptr);
}

void operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
    _aligned_free(ptr);
}

#endif // MEMORY_OVERRIDE
