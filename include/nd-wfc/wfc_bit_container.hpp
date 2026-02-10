#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <bit>
#include <type_traits>
#include <iterator>

#include "wfc_utils.hpp"
#include "wfc_allocator.hpp"
#include "wfc_large_integers.hpp"

namespace WFC {

namespace detail {
    // Helper to determine the optimal storage type based on bits needed
    template<size_t Bits>
    struct OptimalStorageType {
        static constexpr size_t bits_needed = Bits == 0 ? 0 : 
            (Bits <= 1) ? 1 :
            (Bits <= 2) ? 2 :
            (Bits <= 4) ? 4 :
            (Bits <= 8) ? 8 :
            (Bits <= 16) ? 16 :
            (Bits <= 32) ? 32 :
            (Bits <= 64) ? 64 :
            ((Bits + 63) / 64) * 64; // Round up to multiple of 64 for >64 bits
        
        using type = std::conditional_t<bits_needed <= 8, uint8_t,
                     std::conditional_t<bits_needed <= 16, uint16_t,
                     std::conditional_t<bits_needed <= 32, uint32_t,
                     uint64_t>>>;
    };

    // Helper for multi-element storage (>64 bits)
    template<size_t Bits>
    struct StorageArray {
        static constexpr size_t StorageBits = OptimalStorageType<Bits>::bits_needed;
        static constexpr size_t ArraySize = StorageBits > 64 ? (StorageBits / 64) : 1;
        using element_type = std::conditional_t<StorageBits <= 64, typename OptimalStorageType<Bits>::type, uint64_t>;
        using type = std::conditional_t<ArraySize == 1, element_type, LargeInteger<ArraySize>>;
    };

    struct Empty{};
}

template<size_t Bits, size_t Size = 0, typename AllocatorT = WFCStackAllocatorAdapter<typename detail::StorageArray<Bits>::type>>
class BitContainer : private AllocatorT{
public:
    using StorageInfo = detail::OptimalStorageType<Bits>;
    using StorageArrayInfo = detail::StorageArray<Bits>;
    using StorageType = typename StorageArrayInfo::type;
    using AllocatorType = AllocatorT;
    
    static constexpr size_t BitsPerElement = Bits;
    static constexpr size_t StorageBits = StorageInfo::bits_needed;
    static constexpr bool IsResizable = (Size == 0);
    static constexpr bool IsMultiElement = (StorageBits > 64);
    static constexpr bool IsSubByte = (StorageBits < 8);
    static constexpr size_t ElementsPerByte = sizeof(StorageType) * 8 / std::max<size_t>(1u, StorageBits);
    static constexpr size_t MaxValue = (StorageType{1} << BitsPerElement) - 1;
    
    using ContainerType = 
        std::conditional_t<Bits == 0,
            detail::Empty,
            std::conditional_t<IsResizable, 
                std::vector<StorageType, AllocatorType>, 
                std::array<StorageType, Size>>>;

private:
    ContainerType m_container;

private:
    // Mask for extracting bits
    static constexpr auto get_Mask() 
    {
        if constexpr (BitsPerElement == 0) 
        {
            return uint64_t{0};
        } 
        else if constexpr (BitsPerElement >= 64) 
        {
            return ~uint64_t{0};
        } 
        else 
        {
            return (uint64_t{1} << BitsPerElement) - 1;
        }
    }
    
    static constexpr uint64_t Mask = get_Mask();

public:
    static constexpr StorageType GetWaveMask()
    {
        return (StorageType{1} << BitsPerElement) - 1;
    }

    static constexpr StorageType GetMask(std::span<const size_t> indices)
    {
        StorageType mask = 0;
        for (const auto& index : indices) {
            mask |= (StorageType{1} << index);
        }
        return mask;
    }

public:
    
    BitContainer() = default;
    BitContainer(const AllocatorT& allocator) : AllocatorT(allocator) {};
    explicit BitContainer(size_t size, const AllocatorT& allocator) requires (IsResizable) 
        : AllocatorT(allocator)
        , m_container(size, allocator)
    {};
    explicit BitContainer(size_t, const AllocatorT& allocator) requires (!IsResizable)
        : AllocatorT(allocator)
        , m_container()
    {};

public:
    // Size operations
    constexpr size_t size() const noexcept 
    {
        if constexpr (IsResizable) 
        {
            return m_container.size();
        } 
        else 
        {
            return Size;
        }
    }
    constexpr std::span<const StorageType> data() const { return std::span<const StorageType>(m_container); }
    constexpr std::span<StorageType> data() { return std::span<StorageType>(m_container); }
    
    constexpr void resize(size_t new_size) requires (IsResizable) { m_container.resize(new_size); }
    constexpr void reserve(size_t capacity) requires (IsResizable) { m_container.reserve(capacity); }

public: // Sub byte
    struct SubTypeAccess
    {
        constexpr SubTypeAccess(uint8_t& data, uint8_t subIndex) : Data{ data }, Shift{ static_cast<uint8_t>(StorageBits * subIndex) } {};

        constexpr uint8_t GetValue() const { return ((Data >> Shift) & Mask); }
        constexpr uint8_t SetValue(uint8_t val) { Clear(); Data |= ((val & Mask) << Shift); return GetValue(); }
        constexpr void Clear() { Data &= ~(static_cast<uint8_t>(Mask) << Shift); }


        constexpr SubTypeAccess& operator=(uint8_t other) { SetValue(other); return *this; }
        constexpr operator uint8_t() const { return GetValue(); }

        constexpr SubTypeAccess& operator&=(uint8_t other) { SetValue(GetValue() & other); return *this; }
        constexpr SubTypeAccess& operator|=(uint8_t other) { SetValue(GetValue() | other); return *this; }
        constexpr SubTypeAccess& operator^=(uint8_t other) { SetValue(GetValue() ^ other); return *this; }
        constexpr SubTypeAccess& operator<<=(uint8_t other) { SetValue(GetValue() << other); return *this; }
        constexpr SubTypeAccess& operator>>=(uint8_t other) { SetValue(GetValue() >> other); return *this; }

        uint8_t& Data;
        uint8_t Shift;
    };

    constexpr StorageType operator[](size_t index) const requires(IsSubByte) {
        uint8_t shift = static_cast<uint8_t>(StorageBits * (index % ElementsPerByte));
        return (data()[index / ElementsPerByte] >> shift) & static_cast<StorageType>(Mask);
    }
    constexpr SubTypeAccess operator[](size_t index) requires(IsSubByte) { return SubTypeAccess{data()[index / ElementsPerByte], static_cast<uint8_t>(index % ElementsPerByte) }; }

public: // default
    constexpr const StorageType& operator[](size_t index) const requires(!IsSubByte) { return data()[index]; }
    constexpr StorageType& operator[](size_t index) requires(!IsSubByte) { return data()[index]; }

public: // iterators
    template <bool IsConst>
    class BitIterator {
    public:
        // Iterator traits
        using iterator_category = std::random_access_iterator_tag;
        using value_type = StorageType;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const StorageType*, StorageType*>;
        using reference = std::conditional_t<IsConst, const StorageType&, StorageType&>;

    private:
        using ContainerType = std::conditional_t<IsConst, const BitContainer, BitContainer>;
        
        ContainerType* m_container{};
        size_t m_index{};

    public:
        // Constructor
        constexpr BitIterator() = default;
        constexpr BitIterator(ContainerType& container, size_t index) : m_container(&container), m_index(index) {}

        // Dereference
        constexpr reference operator*() const { return (*m_container)[m_index]; }
        constexpr pointer operator->() const { return &(*m_container)[m_index]; }

        // Element access
        constexpr reference operator[](difference_type n) const { return (*m_container)[m_index + n]; }

        // Increment / Decrement
        constexpr BitIterator& operator++() { ++m_index; return *this; }
        constexpr BitIterator operator++(int) { BitIterator tmp = *this; ++m_index; return tmp; }
        constexpr BitIterator& operator--() { --m_index; return *this; }
        constexpr BitIterator operator--(int) { BitIterator tmp = *this; --m_index; return tmp; }

        // Arithmetic
        constexpr BitIterator operator+(difference_type n) const { return BitIterator(*m_container, m_index + n); }
        constexpr BitIterator operator-(difference_type n) const { return BitIterator(*m_container, m_index - n); }
        constexpr difference_type operator-(const BitIterator& other) const { return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index); }

        // Assignment
        constexpr BitIterator& operator+=(difference_type n) { m_index += n; return *this; }
        constexpr BitIterator& operator-=(difference_type n) { m_index -= n; return *this; }

        // Comparison
        constexpr bool operator==(const BitIterator& other) const { return m_index == other.m_index; }
        constexpr bool operator!=(const BitIterator& other) const { return m_index != other.m_index; }
        constexpr bool operator<(const BitIterator& other) const { return m_index < other.m_index; }
        constexpr bool operator>(const BitIterator& other) const { return m_index > other.m_index; }
        constexpr bool operator<=(const BitIterator& other) const { return m_index <= other.m_index; }
        constexpr bool operator>=(const BitIterator& other) const { return m_index >= other.m_index; }

        // Conversion from non-const to const iterator
        constexpr operator BitIterator<true>() const {
            return BitIterator<true>(*m_container, m_index);
        }
    };

    // Type aliases for convenience
    using ConstIterator = BitIterator<true>;
    using Iterator = BitIterator<false>;

    constexpr Iterator begin() { return Iterator{*this, 0}; }
    constexpr Iterator end() { return Iterator{*this, size()}; }
    constexpr const ConstIterator begin() const { return ConstIterator{*this, 0}; }
    constexpr const ConstIterator end() const { return ConstIterator{*this, size()}; }
};

// Free function for iterator addition
template <size_t Bits, size_t Size = 0, typename AllocatorT = WFCStackAllocatorAdapter<typename detail::StorageArray<Bits>::type>, bool IsConst>
BitContainer<Bits, Size, AllocatorT>::BitIterator<IsConst> operator+(
    typename BitContainer<Bits, Size, AllocatorT>::template BitIterator<IsConst>::difference_type n,
    const typename BitContainer<Bits, Size, AllocatorT>::template BitIterator<IsConst>& it) {
    return it + n;
}

static_assert(BitContainer<1, 10>::ElementsPerByte == 8);
static_assert(BitContainer<2, 10>::ElementsPerByte == 4);
static_assert(BitContainer<4, 10>::ElementsPerByte == 2);
static_assert(BitContainer<8, 10>::ElementsPerByte == 1);


} // namespace WFC
