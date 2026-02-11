#pragma once

#include <array>
#include <span>

#include "wfc_variable_map.hpp"
#include "wfc_bit_container.hpp"

namespace WFC {

/**
 * @brief Defines a set of adjacency directions for a topology.
 *
 * Each adjacency is a stateless lambda/callable that takes (const WorldT&, size_t id)
 * and returns the neighbor cell id for that direction.
 *
 * Examples:
 *   2D grid (4 adjacencies): right, left, down, up
 *   3D grid (6 adjacencies): +x, -x, +y, -y, +z, -z
 *   Hex grid (6 adjacencies): user-defined neighbor lookups
 *
 * @tparam WorldT       The world type (e.g. Array2D, Array3D)
 * @tparam Adjacencies  Stateless callable types, each: (const WorldT&, size_t) -> size_t
 */
template <typename WorldT, typename ... Adjacencies>
struct AdjacencyDef {
    static_assert((std::is_empty_v<Adjacencies> && ...), "Adjacency functions must be stateless (no captures)");

    static constexpr size_t Count = sizeof...(Adjacencies);

    using WorldType = WorldT;
    using GetNeighborFnT = size_t(*)(const WorldT&, size_t);

    static constexpr std::array<GetNeighborFnT, Count> Functions = {
        static_cast<GetNeighborFnT>(Adjacencies{})...
    };

    static constexpr size_t GetNeighbor(size_t adjacencyIndex, const WorldT& world, size_t cellId)
    {
        return Functions[adjacencyIndex](world, cellId);
    }
};

// Helper to extract WorldT from an AdjacencyDef instantiation
template <typename T>
struct ExtractWorldType;

template <typename WorldT, typename ... Adjacencies>
struct ExtractWorldType<AdjacencyDef<WorldT, Adjacencies...>> {
    using Type = WorldT;
};

/**
 * @brief An adjacency matrix storing allowed neighbor masks per (direction, cell_type).
 *
 * Layout: AdjacencyCount * VariableCount, each entry is a MaskT bitmask.
 * Entry [dir][varIdx] is a bitmask where bit j is set if variable j is an
 * allowed neighbor of variable varIdx in direction dir.
 *
 * @tparam VariableIDMapT  A VariableIDMap or VariableIDRange mapping cell values to indices
 * @tparam AdjacencyDefT   An AdjacencyDef describing the topology directions
 */
template <typename VariableIDMapT, typename AdjacencyDefT>
class AdjacencyMatrix {
public:
    using VarT = typename VariableIDMapT::Type;
    using WorldT = typename ExtractWorldType<AdjacencyDefT>::Type;
    using MaskT = MinimumBitsType<VariableIDMapT::size()>;

    static constexpr size_t AdjacencyCount = AdjacencyDefT::Count;
    static constexpr size_t VariableCount = VariableIDMapT::size();

public:
    constexpr AdjacencyMatrix() = default;

    /**
     * @brief Get the allowed-neighbor mask for a variable in a given direction
     */
    constexpr MaskT GetMask(size_t adjacency, size_t varIndex) const
    {
        return m_data[adjacency * VariableCount + varIndex];
    }

    /**
     * @brief Set the full allowed-neighbor mask for a variable in a given direction
     */
    constexpr void SetMask(size_t adjacency, size_t varIndex, MaskT mask)
    {
        m_data[adjacency * VariableCount + varIndex] = mask;
    }

    /**
     * @brief Check if neighbor type 'to' is allowed next to 'from' in the given direction
     */
    constexpr bool IsAllowed(size_t adjacency, size_t fromVarIndex, size_t toVarIndex) const
    {
        return (GetMask(adjacency, fromVarIndex) >> toVarIndex) & 1;
    }

    /**
     * @brief Mark that neighbor type 'to' is allowed next to 'from' in the given direction
     */
    constexpr void Allow(size_t adjacency, size_t fromVarIndex, size_t toVarIndex)
    {
        m_data[adjacency * VariableCount + fromVarIndex] |= (MaskT(1) << toVarIndex);
    }

    /**
     * @brief Mark that neighbor type 'to' is disallowed next to 'from' in the given direction
     */
    constexpr void Disallow(size_t adjacency, size_t fromVarIndex, size_t toVarIndex)
    {
        m_data[adjacency * VariableCount + fromVarIndex] &= ~(MaskT(1) << toVarIndex);
    }

    /**
     * @brief Compile-time allow: value From permits value To in the given direction
     */
    template <VarT From, VarT To>
    constexpr void Allow(size_t adjacency)
    {
        Allow(adjacency, VariableIDMapT::template GetIndex<From>(), VariableIDMapT::template GetIndex<To>());
    }

    /**
     * @brief Compile-time disallow: value From forbids value To in the given direction
     */
    template <VarT From, VarT To>
    constexpr void Disallow(size_t adjacency)
    {
        Disallow(adjacency, VariableIDMapT::template GetIndex<From>(), VariableIDMapT::template GetIndex<To>());
    }

    /**
     * @brief Set all masks to all-bits-set (every pair allowed in every direction)
     */
    constexpr void AllowAll()
    {
        MaskT fullMask = (MaskT(1) << VariableCount) - 1;
        for (size_t i = 0; i < AdjacencyCount * VariableCount; ++i)
            m_data[i] = fullMask;
    }

    /**
     * @brief Set all masks to zero (nothing allowed)
     */
    constexpr void DisallowAll()
    {
        for (size_t i = 0; i < AdjacencyCount * VariableCount; ++i)
            m_data[i] = MaskT(0);
    }

    /**
     * @brief Parse an existing world/pattern and populate the adjacency matrix
     *        by observing which (value, neighbor_value) pairs actually occur.
     *
     * For each cell in the world, for each adjacency direction, the neighbor is
     * looked up via the AdjacencyDef lambdas and the pair is marked as allowed.
     *
     * @param world  A populated world instance to read the pattern from
     */
    void BuildFromPattern(const WorldT& world)
    {
        DisallowAll();

        for (size_t cellId = 0; cellId < world.size(); ++cellId)
        {
            VarT cellValue = world.getValue(cellId);
            size_t fromIndex = GetVariableIndex(cellValue);
            if (fromIndex == static_cast<size_t>(-1))
                continue;

            for (size_t dir = 0; dir < AdjacencyCount; ++dir)
            {
                size_t neighborId = AdjacencyDefT::GetNeighbor(dir, world, cellId);

                // Skip self-references (e.g. clamped edges returning the same cell)
                if (neighborId == cellId)
                    continue;

                VarT neighborValue = world.getValue(neighborId);
                size_t toIndex = GetVariableIndex(neighborValue);
                if (toIndex == static_cast<size_t>(-1))
                    continue;

                Allow(dir, fromIndex, toIndex);
            }
        }
    }

private:
    static size_t GetVariableIndex(VarT value)
    {
        auto allValues = VariableIDMapT::GetAllValues();
        for (size_t i = 0; i < VariableIDMapT::size(); ++i)
        {
            if (allValues[i] == value)
                return i;
        }
        return static_cast<size_t>(-1);
    }

private:
    std::array<MaskT, AdjacencyCount * VariableCount> m_data{};
};

} // namespace WFC
