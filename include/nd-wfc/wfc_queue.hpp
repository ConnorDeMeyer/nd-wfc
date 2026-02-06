#pragma once

#include <array>
#include <vector>
#include <type_traits>
#include <concepts>
#include <span>
#include <algorithm>

#include "wfc_utils.hpp"

namespace WFC 
{

template <size_t Size = 0, typename StorageType = size_t>
class WFCQueue {
public:
    using ContainerType = std::conditional_t<Size == 0, std::vector<StorageType>, std::array<StorageType, Size>>;

public:
    WFCQueue() = default;
    WFCQueue(const WFCQueue&) = delete;
    WFCQueue(WFCQueue&&) = delete;
    WFCQueue& operator=(const WFCQueue&) = delete;
    WFCQueue& operator=(WFCQueue&&) = delete;

    constexpr WFCQueue(size_t size)
    {
        if constexpr (Size == 0)
        {
            m_container.resize(size);
        }
    }

public:
    constexpr std::span<const StorageType> data() const { return std::span<const StorageType>(m_container.data(), Size); }
    constexpr std::span<StorageType> data() { return std::span<StorageType>(m_container.data(), Size); }
    
    constexpr std::span<const StorageType> FilledData() const { return std::span<const StorageType>(m_container.data() + m_front, m_back - m_front); }
    constexpr std::span<StorageType> FilledData() { return std::span<StorageType>(m_container.data() + m_front, m_back - m_front); }

    constexpr size_t size() const { return m_container.size(); }

public:
    constexpr bool empty() const { return m_front == m_back; }
    constexpr bool full() const { return m_back == size(); }
    constexpr bool has(StorageType value) const { return std::find(m_container.begin(), m_container.begin() + m_back, value) != m_container.begin() + m_back; }

public:
    constexpr void push(const StorageType &value)
    {
        constexpr_assert(!full());
        constexpr_assert(!has(value));

        m_container[m_back++] = value;
    }

    constexpr StorageType pop() 
    { 
        constexpr_assert(!empty());
        
        return m_container[m_front++];
    }

public:
    struct BranchPoint
    {
        constexpr BranchPoint(WFCQueue<Size, StorageType>& queue)
            : m_queue(queue)
            , m_front(queue.m_front)
            , m_back(queue.m_back)
        {}

        constexpr ~BranchPoint()
        {
            m_queue.m_front = m_front;
            m_queue.m_back = m_back;
        }

        WFCQueue<Size, StorageType>& m_queue;
        size_t m_front;
        size_t m_back;
    };

public:
    constexpr BranchPoint createBranchPoint()
    {
        return BranchPoint(*this);
    }

private:
    ContainerType m_container{};
    size_t m_front = 0;
    size_t m_back = 0;
};

} // namespace WFC