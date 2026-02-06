#pragma once

#include <cstddef>
#include <memory>
#include <cassert>
#include <vector>
#include <queue>
#include <cstring>

#include <cstdlib>
#include <memory>

#include "wfc_utils.hpp"

namespace WFC {

/**
 * @brief Stack allocator specifically designed for WFC branching operations
 *
 * This allocator uses a stack-based approach where memory is allocated in chunks
 * and automatically deallocated when branches go out of scope. It's optimized
 * for the recursive branching pattern used in the WFC algorithm.
 */
class WFCStackAllocator {
private:
    struct MemoryPool {
        void* ptr;
        size_t size;
        size_t used;

        MemoryPool() : ptr(nullptr), size(0), used(0) {}
        MemoryPool(void* p, size_t s) : ptr(p), size(s), used(0) {}
    };

public:
    /**
     * @brief Construct allocator with initial capacity
     * @param initialCapacity Initial memory pool size in bytes
     */
    explicit WFCStackAllocator(size_t initialCapacity = 1024 * 1024) // 1MB default
    {
        addPool(0); // first pool is 0
        addPool(initialCapacity);
        m_currentPoolIndex = 1;
    }

    explicit WFCStackAllocator(std::span<uint8_t> userGivenData)
    {
        m_pools.push_back(MemoryPool(userGivenData.data(), userGivenData.size()));
        m_currentPoolIndex = 0;
    }

    ~WFCStackAllocator() {
        // first pool is not deallocated because it's either empty or comes from the user
        for (size_t i = 1; i < m_pools.size(); ++i)
        {
            delete[] static_cast<char*>(m_pools[i].ptr);
        }
    }

    // Non-copyable, non-movable for safety
    WFCStackAllocator(const WFCStackAllocator&) = delete;
    WFCStackAllocator& operator=(const WFCStackAllocator&) = delete;
    WFCStackAllocator(WFCStackAllocator&&) = delete;
    WFCStackAllocator& operator=(WFCStackAllocator&&) = delete;

    void* allocate(size_t size) 
    {
        size = alignUp(size);

        for (; m_currentPoolIndex < m_pools.size(); ++m_currentPoolIndex)
        {
            if (getCapacity() >= size) 
            {
                auto& pool = m_pools[m_currentPoolIndex];
                void* ptr = static_cast<char*>(pool.ptr) + pool.used;
                pool.used += size;
                return ptr;
            }
        }


        // No existing pool has enough space, add a new one
        size_t newPoolSize = std::max(m_pools.back().size * 2, size * 2); // Grow exponentially
        addPool(newPoolSize);

        return allocate(size);
    }

    void deallocate(void*) 
    {}

    /**
     * @brief Create a stack frame marker for RAII-based cleanup
     * @return StackFrame object that will cleanup on destruction
     */
    class StackFrame {
    private:
        WFCStackAllocator& m_allocator;
        size_t m_poolIndex{};
        size_t m_poolUsed{};

    public:
        StackFrame(WFCStackAllocator& allocator)
            : m_allocator(allocator)
            , m_poolIndex(allocator.m_currentPoolIndex)
            , m_poolUsed(allocator.m_pools[m_poolIndex].used)
        {}

        ~StackFrame() 
        {
            for (size_t i = m_allocator.m_pools.size() - 1; i > m_poolIndex; --i)
            {
                m_allocator.m_pools[i].used = 0;
            }

            m_allocator.m_pools[m_poolIndex].used = m_poolUsed;
            m_allocator.m_currentPoolIndex = m_poolIndex;
        }

        // Non-copyable, movable
        StackFrame(const StackFrame&) = delete;
        StackFrame& operator=(const StackFrame&) = delete;
        StackFrame(StackFrame&&) = default;
        StackFrame& operator=(StackFrame&&) = default;
    };

    /**
     * @brief Create a new stack frame for a branch
     */
    StackFrame createFrame()
    {
        return StackFrame(*this);
    }

    constexpr size_t getCapacity() const 
    {
        return m_pools[m_currentPoolIndex].size - m_pools[m_currentPoolIndex].used;
    }

    static constexpr size_t alignUp(size_t value) 
    {
        return (value + 8 - 1) & ~(8 - 1);
    }

private:
    constexpr void addPool(size_t size)
    {
        void* ptr = new char[size]; // Allocate bytes
        if (!ptr) {
            throw std::bad_alloc();
        }
        m_pools.emplace_back(ptr, size);
    }

private:
    std::vector<MemoryPool> m_pools;
    size_t m_currentPoolIndex{};
};

/**
 * @brief Custom allocator adapter for STL containers using WFCStackAllocator
 */
template<typename T>
class WFCStackAllocatorAdapter {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = WFCStackAllocatorAdapter<U>;
    };

    WFCStackAllocatorAdapter(WFCStackAllocator& allocator) 
        : m_allocator(&allocator) {}

    template<typename U>
    WFCStackAllocatorAdapter(const WFCStackAllocatorAdapter<U>& other) 
        : m_allocator(other.m_allocator) {}

    pointer allocate(size_type n) {
        size_t size = n * sizeof(T);
        size_t alignment = alignof(T);
        // Ensure alignment
        size = (size + alignment - 1) & ~(alignment - 1);
        return static_cast<pointer>(m_allocator->allocate(size));
    }

    void deallocate(pointer ptr, size_type) {
        m_allocator->deallocate(ptr);
    }

    template<typename U>
    bool operator==(const WFCStackAllocatorAdapter<U>& other) const {
        return m_allocator == other.m_allocator;
    }

    template<typename U>
    bool operator!=(const WFCStackAllocatorAdapter<U>& other) const {
        return !(*this == other);
    }

    WFCStackAllocator* m_allocator;
};

} // namespace WFC
