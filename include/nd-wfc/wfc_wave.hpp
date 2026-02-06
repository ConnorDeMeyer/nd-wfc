#pragma once

#include "wfc_bit_container.hpp"
#include "wfc_variable_map.hpp"
#include "wfc_allocator.hpp"

namespace WFC {

template <typename VariableIDMapT, size_t Size = 0>
class Wave {
public:
    using BitContainerT = BitContainer<VariableIDMapT::size(), Size>;
    using ElementT = typename BitContainerT::StorageType;
    using IDMapT = VariableIDMapT;
    using VariableIDT = typename VariableIDMapT::VariableIDT;

    static constexpr size_t ElementsAmount = Size;

public:
    Wave() = default;
    Wave(size_t size, size_t variableAmount, WFCStackAllocator& allocator) : m_data(size, allocator) 
    {
        for (auto& wave : m_data) wave = (1 << variableAmount) - 1;
    }

    Wave(const Wave& other) = default;

public:
    void Collapse(size_t index, ElementT mask) { m_data[index] &= mask; }
    size_t size() const { return m_data.size(); }
    size_t Entropy(size_t index) const { return std::popcount(m_data[index]); }
    bool IsCollapsed(size_t index) const { return Entropy(index) == 1; }
    bool IsFullyCollapsed() const { return std::all_of(m_data.begin(), m_data.end(), [](ElementT value) { return std::popcount(value) == 1; }); }
    bool HasContradiction() const { return std::any_of(m_data.begin(), m_data.end(), [](ElementT value) { return value == 0; }); }
    bool IsContradicted(size_t index) const { return m_data[index] == 0; }
    uint16_t GetVariableID(size_t index) const { return static_cast<uint16_t>(std::countr_zero(m_data[index])); }
    ElementT GetMask(size_t index) const { return m_data[index]; }

private:
    BitContainerT m_data;
};

}