#pragma once

namespace WFC
{




# ifdef _DEBUG
inline constexpr void constexpr_assert(bool condition, const char* message = "") 
{
    if (!condition) throw message;
}
#else
inline constexpr void constexpr_assert(bool condition, const char* message = "") 
{
    (void)condition;
    (void)message;
}
# endif

template <size_t Size>
using MinimumIntegerType =  std::conditional_t<Size <= std::numeric_limits<uint8_t>::max(), uint8_t, 
                            std::conditional_t<Size <= std::numeric_limits<uint16_t>::max(), uint16_t, 
                            std::conditional_t<Size <= std::numeric_limits<uint32_t>::max(), uint32_t, 
                            uint64_t>>>;

template <uint8_t bits>
using MinimumBitsType =     std::conditional_t<bits <= 8, uint8_t, 
                            std::conditional_t<bits <= 16, uint16_t, 
                            std::conditional_t<bits <= 32, uint32_t,
                            std::conditional_t<bits <= 64, uint64_t,
                            void>>>>;


inline int FindNthSetBit(size_t num, int n) {
    constexpr_assert(n < std::popcount(num), "index is out of range");
    int bitCount = 0;
    while (num) {
        if (bitCount == n) {
            return std::countr_zero(num); // Index of the current set bit
        }
        bitCount++;
        num &= (num - 1); // turn of lowest set bit
    }
    return bitCount;
}

template <typename VarT>
struct WorldValue 
{
public:
    WorldValue() = default;
    WorldValue(VarT value, uint16_t internalIndex)
        : Value(value)
        , InternalIndex(internalIndex)
    {}
public:
    operator VarT() const { return Value; }

public:
    VarT Value{};
    uint16_t InternalIndex{};
};

}