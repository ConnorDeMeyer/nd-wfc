#pragma once

#include <array>
#include <bit>
#include <limits>
#include <algorithm>
#include <type_traits>
#include <stdexcept>

// Detect __uint128_t support
#if (defined(__SIZEOF_INT128__) || defined(__INTEL_COMPILER) || (defined(__GNUC__) && __GNUC__ >= 4)) && !defined(_MSC_VER)
#define WFC_HAS_UINT128 1
#else
#define WFC_HAS_UINT128 0
#endif

namespace WFC {

template <size_t Size>
struct LargeInteger
{
    static_assert(Size > 0, "Size must be greater than 0");

    std::array<uint64_t, Size> m_data;

    // Constructors
    constexpr LargeInteger() = default;
    constexpr LargeInteger(const LargeInteger&) = default;
    constexpr LargeInteger(LargeInteger&&) = default;
    constexpr LargeInteger& operator=(const LargeInteger&) = default;
    constexpr LargeInteger& operator=(LargeInteger&&) = default;

    // Constructor from uint64_t (for small values)
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>>>
    constexpr explicit LargeInteger(T value) {
        m_data.fill(0);
        if constexpr (sizeof(T) <= sizeof(uint64_t)) {
            m_data[0] = static_cast<uint64_t>(value);
        } else {
            // Handle larger types if needed
            static_assert(sizeof(T) <= sizeof(uint64_t), "Type too large for LargeInteger");
        }
    }

    // Access operators
    constexpr uint64_t& operator[](size_t index) { return m_data[index]; }
    constexpr const uint64_t& operator[](size_t index) const { return m_data[index]; }

    // Helper function to get the larger size type
    template <size_t OtherSize>
    using LargerType = LargeInteger<std::max(Size, OtherSize)>;

    // Helper function to promote operands to the same size
    template <size_t OtherSize>
    constexpr auto promote(const LargeInteger<OtherSize>& other) const {
        constexpr size_t ResultSize = std::max(Size, OtherSize);
        LargeInteger<ResultSize> lhs_promoted{};
        LargeInteger<ResultSize> rhs_promoted{};

        // Copy data, padding with zeros
        for (size_t i = 0; i < Size; ++i) {
            lhs_promoted[i] = m_data[i];
        }
        for (size_t i = 0; i < OtherSize; ++i) {
            rhs_promoted[i] = other[i];
        }

        return std::make_pair(lhs_promoted, rhs_promoted);
    }

    // Arithmetic operators
    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator+(const LargeInteger<OtherSize>& other) const {
        auto [lhs, rhs] = promote(other);
        constexpr size_t ResultSize = std::max(Size, OtherSize);
        LargeInteger<ResultSize> result{};

        uint64_t carry = 0;
        for (size_t i = 0; i < ResultSize; ++i) {
            uint64_t sum = lhs[i] + rhs[i] + carry;
            result[i] = sum;
            carry = (sum < lhs[i] || (carry && sum == lhs[i])) ? 1 : 0;
        }

        return result;
    }

    template <size_t OtherSize>
    constexpr LargeInteger& operator+=(const LargeInteger<OtherSize>& other) {
        *this = *this + other;
        return *this;
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator-(const LargeInteger<OtherSize>& other) const {
        auto [lhs, rhs] = promote(other);
        constexpr size_t ResultSize = std::max(Size, OtherSize);
        LargeInteger<ResultSize> result{};

        uint64_t borrow = 0;
        for (size_t i = 0; i < ResultSize; ++i) {
            uint64_t diff = lhs[i] - rhs[i] - borrow;
            result[i] = diff;
            borrow = (lhs[i] < rhs[i] + borrow) ? 1 : 0;
        }

        return result;
    }

    template <size_t OtherSize>
    constexpr LargeInteger& operator-=(const LargeInteger<OtherSize>& other) {
        *this = *this - other;
        return *this;
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator*(const LargeInteger<OtherSize>& other) const {
#if WFC_HAS_UINT128
        auto [lhs, rhs] = promote(other);
        constexpr size_t ResultSize = std::max(Size, OtherSize);
        LargeInteger<ResultSize * 2> result{}; // Multiplication can double the size

        for (size_t i = 0; i < ResultSize; ++i) {
            uint64_t carry = 0;
            for (size_t j = 0; j < ResultSize; ++j) {
                __uint128_t product = static_cast<__uint128_t>(lhs[i]) * rhs[j] + result[i + j] + carry;
                result[i + j] = static_cast<uint64_t>(product);
                carry = product >> 64;
            }
            size_t k = i + ResultSize;
            while (carry && k < ResultSize * 2) {
                __uint128_t sum = result[k] + carry;
                result[k] = static_cast<uint64_t>(sum);
                carry = sum >> 64;
                ++k;
            }
        }

        // Truncate to the larger of the original sizes
        LargeInteger<ResultSize> final_result{};
        for (size_t i = 0; i < ResultSize; ++i) {
            final_result[i] = result[i];
        }
        return final_result;
#else
        throw std::runtime_error("LargeInteger multiplication requires __uint128_t support, which is not available on this compiler/platform");
#endif
    }

    template <size_t OtherSize>
    constexpr LargeInteger& operator*=(const LargeInteger<OtherSize>& other) {
        *this = *this * other;
        return *this;
    }

    // Division and modulo (simplified implementation)
    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator/(const LargeInteger<OtherSize>& other) const {
        // Simplified division - assumes other is not zero and result fits
        auto [lhs, rhs] = promote(other);
        constexpr size_t ResultSize = std::max(Size, OtherSize);
        LargeInteger<ResultSize> result{};

        // This is a very basic division implementation
        // For a full implementation, you'd need proper long division
        LargeInteger<ResultSize> temp = lhs;
        while (temp >= rhs) {
            temp = temp - rhs;
            result = result + LargeInteger<ResultSize>{1};
        }

        return result;
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator%(const LargeInteger<OtherSize>& other) const {
        auto [lhs, rhs] = promote(other);
        constexpr size_t ResultSize = std::max(Size, OtherSize);
        LargeInteger<ResultSize> temp = lhs;
        while (temp >= rhs) {
            temp = temp - rhs;
        }
        return temp;
    }

    // Unary operators
    constexpr LargeInteger operator-() const {
        LargeInteger result{};
        for (size_t i = 0; i < Size; ++i) {
            result[i] = ~m_data[i] + 1; // Two's complement
        }
        return result;
    }

    constexpr LargeInteger operator~() const {
        LargeInteger result{};
        for (size_t i = 0; i < Size; ++i) {
            result[i] = ~m_data[i];
        }
        return result;
    }

    // Bit operations
    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator&(const LargeInteger<OtherSize>& other) const {
        auto [lhs, rhs] = promote(other);
        return lhs.bitwise_op(rhs, std::bit_and<uint64_t>{});
    }

    template <size_t OtherSize>
    constexpr LargeInteger& operator&=(const LargeInteger<OtherSize>& other) {
        *this = *this & other;
        return *this;
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator|(const LargeInteger<OtherSize>& other) const {
        auto [lhs, rhs] = promote(other);
        return lhs.bitwise_op(rhs, std::bit_or<uint64_t>{});
    }

    template <size_t OtherSize>
    constexpr LargeInteger& operator|=(const LargeInteger<OtherSize>& other) {
        *this = *this | other;
        return *this;
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator^(const LargeInteger<OtherSize>& other) const {
        auto [lhs, rhs] = promote(other);
        return lhs.bitwise_op(rhs, std::bit_xor<uint64_t>{});
    }

    template <size_t OtherSize>
    constexpr LargeInteger& operator^=(const LargeInteger<OtherSize>& other) {
        *this = *this ^ other;
        return *this;
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator<<(size_t shift) const {
        constexpr size_t ResultSize = std::max(Size, OtherSize);
        LargeInteger<ResultSize> result = *this;

        size_t word_shift = shift / 64;
        size_t bit_shift = shift % 64;

        if (word_shift >= ResultSize) {
            result.m_data.fill(0);
            return result;
        }

        // Shift words
        for (size_t i = ResultSize - 1; i >= word_shift; --i) {
            result[i] = result[i - word_shift];
        }
        for (size_t i = 0; i < word_shift; ++i) {
            result[i] = 0;
        }

        // Shift bits
        if (bit_shift > 0) {
            uint64_t carry = 0;
            for (size_t i = word_shift; i < ResultSize; ++i) {
                uint64_t new_carry = result[i] >> (64 - bit_shift);
                result[i] = (result[i] << bit_shift) | carry;
                carry = new_carry;
            }
        }

        return result;
    }

    template <size_t OtherSize>
    constexpr LargeInteger& operator<<=(size_t shift) {
        *this = *this << shift;
        return *this;
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> operator>>(size_t shift) const {
        constexpr size_t ResultSize = std::max(Size, OtherSize);
        LargeInteger<ResultSize> result = *this;

        size_t word_shift = shift / 64;
        size_t bit_shift = shift % 64;

        if (word_shift >= ResultSize) {
            result.m_data.fill(0);
            return result;
        }

        // Shift words
        for (size_t i = 0; i < ResultSize - word_shift; ++i) {
            result[i] = result[i + word_shift];
        }
        for (size_t i = ResultSize - word_shift; i < ResultSize; ++i) {
            result[i] = 0;
        }

        // Shift bits
        if (bit_shift > 0) {
            uint64_t carry = 0;
            for (size_t i = ResultSize - word_shift - 1; i < ResultSize; --i) {
                uint64_t new_carry = result[i] << (64 - bit_shift);
                result[i] = (result[i] >> bit_shift) | carry;
                carry = new_carry;
                if (i == 0) break;
            }
        }

        return result;
    }

    template <size_t OtherSize>
    constexpr LargeInteger& operator>>=(size_t shift) {
        *this = *this >> shift;
        return *this;
    }

    // Comparison operators
    template <size_t OtherSize>
    constexpr bool operator==(const LargeInteger<OtherSize>& other) const {
        auto [lhs, rhs] = promote(other);
        return lhs.m_data == rhs.m_data;
    }

    template <size_t OtherSize>
    constexpr bool operator!=(const LargeInteger<OtherSize>& other) const {
        return !(*this == other);
    }

    template <size_t OtherSize>
    constexpr bool operator<(const LargeInteger<OtherSize>& other) const {
        auto [lhs, rhs] = promote(other);
        for (size_t i = lhs.m_data.size(); i > 0; --i) {
            if (lhs.m_data[i-1] != rhs.m_data[i-1]) {
                return lhs.m_data[i-1] < rhs.m_data[i-1];
            }
        }
        return false;
    }

    template <size_t OtherSize>
    constexpr bool operator<=(const LargeInteger<OtherSize>& other) const {
        return *this < other || *this == other;
    }

    template <size_t OtherSize>
    constexpr bool operator>(const LargeInteger<OtherSize>& other) const {
        return other < *this;
    }

    template <size_t OtherSize>
    constexpr bool operator>=(const LargeInteger<OtherSize>& other) const {
        return other <= *this;
    }

    // std::bit library functions
    constexpr int countl_zero() const {
        for (size_t i = Size; i > 0; --i) {
            if (m_data[i-1] != 0) {
                return std::countl_zero(m_data[i-1]) + (Size - i) * 64;
            }
        }
        return Size * 64;
    }

    constexpr int countl_one() const {
        for (size_t i = Size; i > 0; --i) {
            if (m_data[i-1] != std::numeric_limits<uint64_t>::max()) {
                return std::countl_one(m_data[i-1]) + (Size - i) * 64;
            }
        }
        return Size * 64;
    }

    constexpr int countr_zero() const {
        for (size_t i = 0; i < Size; ++i) {
            if (m_data[i] != 0) {
                return std::countr_zero(m_data[i]) + i * 64;
            }
        }
        return Size * 64;
    }

    constexpr int countr_one() const {
        for (size_t i = 0; i < Size; ++i) {
            if (m_data[i] != std::numeric_limits<uint64_t>::max()) {
                return std::countr_one(m_data[i]) + i * 64;
            }
        }
        return Size * 64;
    }

    constexpr int popcount() const {
        int count = 0;
        for (size_t i = 0; i < Size; ++i) {
            count += std::popcount(m_data[i]);
        }
        return count;
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> rotl(size_t shift) const {
        shift %= (Size * 64);
        return (*this << shift) | (*this >> ((Size * 64) - shift));
    }

    template <size_t OtherSize>
    constexpr LargerType<OtherSize> rotr(size_t shift) const {
        shift %= (Size * 64);
        return (*this >> shift) | (*this << ((Size * 64) - shift));
    }

    constexpr bool has_single_bit() const {
        return popcount() == 1;
    }

    constexpr LargeInteger bit_ceil() const {
        if (*this == LargeInteger{0}) return LargeInteger{1};

        LargeInteger result = *this;
        result -= LargeInteger{1};
        result |= result >> 1;
        result |= result >> 2;
        result |= result >> 4;
        result |= result >> 8;
        result |= result >> 16;
        result |= result >> 32;

        // Handle multi-word case
        for (size_t i = 1; i < Size; ++i) {
            if (result[i] != 0) {
                // Find the highest set bit in the higher words
                size_t highest_word = Size - 1;
                for (size_t j = Size - 1; j > 0; --j) {
                    if (result[j] != 0) {
                        highest_word = j;
                        break;
                    }
                }
                // Set all lower words to 0 and the highest word to the power of 2
                for (size_t j = 0; j < highest_word; ++j) {
                    result[j] = 0;
                }
                result[highest_word] = uint64_t(1) << (63 - std::countl_zero(result[highest_word]));
                break;
            }
        }

        result += LargeInteger{1};
        return result;
    }

    constexpr LargeInteger bit_floor() const {
        if (*this == LargeInteger{0}) return LargeInteger{0};

        LargeInteger result = *this;
        result |= result >> 1;
        result |= result >> 2;
        result |= result >> 4;
        result |= result >> 8;
        result |= result >> 16;
        result |= result >> 32;

        // Handle multi-word case
        for (size_t i = 1; i < Size; ++i) {
            if (result[i] != 0) {
                size_t highest_word = Size - 1;
                for (size_t j = Size - 1; j > 0; --j) {
                    if (result[j] != 0) {
                        highest_word = j;
                        break;
                    }
                }
                for (size_t j = 0; j < highest_word; ++j) {
                    result[j] = 0;
                }
                result[highest_word] = uint64_t(1) << (63 - std::countl_zero(result[highest_word]));
                return result;
            }
        }

        // Single word case
        result = LargeInteger{uint64_t(1) << (63 - std::countl_zero(result[0]))};
        return result;
    }

    constexpr int bit_width() const {
        if (*this == LargeInteger{0}) return 0;

        for (size_t i = Size; i > 0; --i) {
            if (m_data[i-1] != 0) {
                return (i - 1) * 64 + 64 - std::countl_zero(m_data[i-1]);
            }
        }
        return 0;
    }

private:
    // Helper function for bitwise operations
    template <typename Op>
    constexpr LargeInteger bitwise_op(const LargeInteger& other, Op op) const {
        LargeInteger result{};
        for (size_t i = 0; i < Size; ++i) {
            result[i] = op(m_data[i], other[i]);
        }
        return result;
    }
};

// Deduction guide for constructor from integral types
template <typename T>
LargeInteger(T) -> LargeInteger<1>;

} // namespace WFC