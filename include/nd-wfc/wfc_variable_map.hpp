#pragma once

#include <concepts>
#include "wfc_utils.hpp"

namespace WFC {

/**
* @brief Class to map variable values to indices at compile time
* 
* This class is used to map variable values to indices at compile time.
* It is a compile-time map of variable values to indices.
*/

template <size_t VariablesAmount>
using VariableIDType = std::conditional_t<VariablesAmount <= std::numeric_limits<uint8_t>::max(), uint8_t, uint16_t>;


template <typename VarT, VarT ... Values>
class VariableIDMap {
public:

    template <VarT ... AdditionalValues>
    using Merge = VariableIDMap<VarT, Values..., AdditionalValues...>;

    using VariableIDT = VariableIDType<sizeof...(Values)>;

    template <VarT Value>
    static consteval bool HasValue()
    {
        constexpr VarT arr[] = {Values...};
        
        for (size_t i = 0; i < size(); ++i)
            if (arr[i] == Value)
                return true;
        return false;
    }

    template <VarT Value>
    static consteval size_t GetIndex()
    {
        static_assert(HasValue<Value>(), "Value was not defined");
        constexpr VarT arr[] = {Values...};
        
        for (size_t i = 0; i < size(); ++i)
            if (arr[i] == Value)
                return i;
        
        return static_cast<size_t>(-1); // This line is unreachable if value is found
    }

    static std::span<const VarT> GetAllValues()
    {
        static const VarT allValues[]
        {
            Values...
        };
        return std::span<const VarT>{ allValues, size() };
    }

    static constexpr VarT GetValue(size_t index) {
        constexpr_assert(index < size());
        return GetAllValues()[index];
    }

    static consteval size_t size() { return sizeof...(Values); }

    template <VarT ... ValuesSlice>
    static constexpr auto ValuesToIndices() -> std::array<size_t, sizeof...(ValuesSlice)> {
        std::array<size_t, sizeof...(ValuesSlice)> indices = {GetIndex<ValuesSlice>()...};
        return indices;
    }
};

template <typename VarT, size_t Start, size_t End>
class VariableIDRange
{
public:
    using Type = VarT;
    using VariableIDT = VariableIDType<End - Start>;

    static_assert(Start < End, "Start must be less than End");
    static_assert(std::numeric_limits<VarT>::min() <= Start, "VarT must be able to represent all values in the range");
    static_assert(std::numeric_limits<VarT>::max() >= End, "VarT must be able to represent all values in the range");

    static constexpr size_t size() { return End - Start; }

    template <VarT Value>
    static consteval bool HasValue()
    {
        return Value >= Start && Value < End;
    }

    template <VarT Value>
    static consteval size_t GetIndex()
    {
        return Value - Start;
    }

    static constexpr VarT GetValue(size_t index) 
    {
        return Start + index;
    }

    template <VarT ... ValuesSlice>
    static constexpr auto ValuesToIndices() -> std::array<size_t, sizeof...(ValuesSlice)> {
        std::array<size_t, sizeof...(ValuesSlice)> indices = {GetIndex<ValuesSlice>()...};
        return indices;
    }
};

 
}