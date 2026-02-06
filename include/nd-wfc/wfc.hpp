#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <random>
#include <optional>
#include <type_traits>
#include <cassert>
#include <algorithm>
#include <ranges>
#include <concepts>
#include <bit>
#include <span>
#include <tuple>

#include "wfc_utils.hpp"
#include "wfc_variable_map.hpp"
#include "wfc_allocator.hpp"
#include "wfc_bit_container.hpp"
#include "wfc_wave.hpp"
#include "wfc_constrainer.hpp"
#include "wfc_callbacks.hpp"
#include "wfc_random.hpp"
#include "wfc_queue.hpp"

namespace WFC {

template<typename T>
concept WorldType = requires(T world, typename T::ValueType value) {
    { world.size() } -> std::integral;
    { world.setValue(static_cast<decltype(world.size())>(0), value) };
    { world.getValue(static_cast<decltype(world.size())>(0)) } -> std::convertible_to<typename T::ValueType>;
    typename T::ValueType;
};

template <typename T, typename WorldT, typename VarT, typename VariableIDMapT, typename PropagationQueueType>
concept ConstrainerFunction = requires(T func, WorldT& world, size_t index, WorldValue<VarT> value, Constrainer<VariableIDMapT, PropagationQueueType>& constrainer) {
    func(world, index, value, constrainer);
};

template <typename WorldT>
concept HasConstexprSize = requires {
    { []() constexpr -> std::size_t { return WorldT{}.size(); }() };
};

template<typename WorldT, typename VarT,
    typename VariableIDMapT = VariableIDMap<VarT>,
    typename ConstrainerFunctionMapT = ConstrainerFunctionMap<void*>,
    typename CallbacksT = Callbacks<WorldT>,
    typename RandomSelectorT = DefaultRandomSelector<VarT>
>
class WFC {
public:
    static_assert(WorldType<WorldT>, "WorldT must satisfy World type requirements");

    using WorldSizeT = decltype(WorldT{}.size());

    // Try getting the world size, which is only available if the world type has a constexpr size() method
    constexpr static WorldSizeT WorldSize = HasConstexprSize<WorldT> ? WorldT{}.size() : 0;

    using WaveType = Wave<VariableIDMapT, WorldSize>;
    using PropagationQueueType = WFCQueue<WorldSize, WorldSizeT>;
    using ConstrainerType = Constrainer<WaveType, PropagationQueueType>;
    using MaskType = typename WaveType::ElementT;
    using VariableIDT = typename WaveType::VariableIDT;

public:
    struct SolverState
    {
        WorldT& m_world;
        PropagationQueueType m_propagationQueue{};
        RandomSelectorT m_randomSelector{};
        WFCStackAllocator m_allocator{};
        size_t m_iterations{};

        SolverState(WorldT& world, uint32_t seed)
            : m_world(world)
            , m_propagationQueue{ WorldSize ? WorldSize : static_cast<WorldSizeT>(world.size()) }
            , m_randomSelector(seed)
        {}

        SolverState(const SolverState& other) = default;
    };

public:
    WFC() = delete; // dont make an instance of this class, only use the static methods.

public:

    static bool Run(WorldT& world, uint32_t seed = std::random_device{}())
    {
        SolverState state{ world, seed };
        bool result = Run(state);

        return result;
    }

    /**
    * @brief Run the WFC algorithm to generate a solution
    * @return true if a solution was found, false if contradiction occurred
    */
    static bool Run(SolverState& state)
    {
        WaveType wave{ WorldSize, VariableIDMapT::size(), state.m_allocator };

        PropogateInitialValues(state, wave);

        if (RunLoop(state, wave)) {

            PopulateWorld(state, wave);
            return true;
        }
        return false;
    }

    static bool RunLoop(SolverState& state, WaveType& wave)
    {
        static constexpr size_t MaxIterations = 1024 * 8;
        for (; state.m_iterations < MaxIterations; ++state.m_iterations)
        {
            if (!Propagate(state, wave))
                return false;

            if (wave.HasContradiction())
            {
                if constexpr (CallbacksT::HasContradictionCallback())
                {
                    PopulateWorld(state, wave);
                    typename CallbacksT::ContradictionCallback{}(state.m_world);
                }
                return false;
            }

            if (wave.IsFullyCollapsed())
                return true;

            if constexpr (CallbacksT::HasBranchCallback())
            {
                PopulateWorld(state, wave);
                typename CallbacksT::BranchCallback{}(state.m_world);
            }

            if (Branch(state, wave))
                return true;
        }
        return false;
    }

    /**
    * @brief Get the value at a specific cell
    * @param cellId The cell ID
    * @return The value if collapsed, std::nullopt otherwise
    */
    static std::optional<VarT> GetValue(WaveType& wave, int cellId) {
        if (wave.IsCollapsed(cellId)) {
            auto variableId = wave.GetVariableID(cellId);
            return VariableIDMapT::GetValue(variableId);
        }
        return std::nullopt;
    }

    /**
    * @brief Get all possible values for a cell
    * @param cellId The cell ID
    * @return Set of possible values
    */
    static const std::vector<VarT> GetPossibleValues(WaveType& wave, int cellId) 
    {
        std::vector<VarT> possibleValues;
        MaskType mask = wave.GetMask(cellId);
        for (size_t i = 0; i < ConstrainerFunctionMapT::size(); ++i) {
            if (mask & (1 << i)) possibleValues.push_back(VariableIDMapT::GetValue(i));
        }
        return possibleValues;
    }

private:
    static void CollapseCell(SolverState& state, WaveType& wave, WorldSizeT cellId, VariableIDT value)
    {
        constexpr_assert(!wave.IsCollapsed(cellId) || wave.GetMask(cellId) == (MaskType(1) << value));
        wave.Collapse(cellId, 1 << value);
        constexpr_assert(wave.IsCollapsed(cellId));
        
        if constexpr (CallbacksT::HasCellCollapsedCallback())
        {
            PopulateWorld(state, wave);
            typename CallbacksT::CellCollapsedCallback{}(state.m_world);
        }
    }

    static bool Branch(SolverState& state, WaveType& wave)
    {
        constexpr_assert(state.m_propagationQueue.empty());

        // Find cell with minimum entropy > 1
        WorldSizeT minEntropyCell = static_cast<WorldSizeT>(-1);
        size_t minEntropy = static_cast<size_t>(-1);

        for (WorldSizeT i = 0; i < wave.size(); ++i) {
            size_t entropy = wave.Entropy(i);
            if (entropy > 1 && entropy < minEntropy) {
                minEntropy = entropy;
                minEntropyCell = i;
            }
        }
        if (minEntropyCell == static_cast<WorldSizeT>(-1)) return false;

        constexpr_assert(!wave.IsCollapsed(minEntropyCell));

        // create a list of possible values
        VariableIDT availableValues = static_cast<VariableIDT>(wave.Entropy(minEntropyCell));
        std::array<VariableIDT, VariableIDMapT::size()> possibleValues{};
        MaskType mask = wave.GetMask(minEntropyCell);
        for (size_t i = 0; i < availableValues; ++i)
        {
            VariableIDT index = static_cast<VariableIDT>(std::countr_zero(mask)); // get the index of the lowest set bit
            constexpr_assert(index < VariableIDMapT::size(), "Possible value went outside bounds");

            possibleValues[i] = index;
            constexpr_assert(((mask & (MaskType(1) << index)) != 0), "Possible value was not set");

            mask = mask & (mask - 1); // turn off lowest set bit
        }

        // randomly select a value from possible values
        while (availableValues)
        {
            size_t randomIndex = state.m_randomSelector.rng(availableValues);
            VariableIDT selectedValue = possibleValues[randomIndex];

            {
                // copy the state and branch out
                auto stackFrame = state.m_allocator.createFrame();
                auto queueFrame = state.m_propagationQueue.createBranchPoint();

                auto newWave = wave;
                CollapseCell(state, newWave, minEntropyCell, selectedValue);
                state.m_propagationQueue.push(minEntropyCell);

                if (RunLoop(state, newWave))
                {
                    // move the solution to the original state
                    wave = newWave;

                    return true;
                }
            }

            // remove the failure state from the wave
            constexpr_assert((wave.GetMask(minEntropyCell) & (MaskType(1) << selectedValue)) != 0, "Possible value was not set");
            wave.Collapse(minEntropyCell, ~(1 << selectedValue));
            constexpr_assert((wave.GetMask(minEntropyCell) & (MaskType(1) << selectedValue)) == 0, "Wave was not collapsed correctly");

            // swap replacement value with the last value
            std::swap(possibleValues[randomIndex], possibleValues[--availableValues]);
        }

        return false;
    }

    static bool Propagate(SolverState& state, WaveType& wave)
    {
        while (!state.m_propagationQueue.empty())
        {
            WorldSizeT cellId = state.m_propagationQueue.pop();

            if (wave.IsContradicted(cellId)) return false;

            constexpr_assert(wave.IsCollapsed(cellId), "Cell was not collapsed");

            VariableIDT variableID = wave.GetVariableID(cellId);
            ConstrainerType constrainer(wave, state.m_propagationQueue);

            using ConstrainerFunctionPtrT = void(*)(WorldT&, size_t, WorldValue<VarT>, ConstrainerType&);

            ConstrainerFunctionMapT::template GetFunction<ConstrainerFunctionPtrT>(variableID)(state.m_world, cellId, WorldValue<VarT>{VariableIDMapT::GetValue(variableID), variableID}, constrainer);
        }
        return true;
    }    
    
    static void PopulateWorld(SolverState& state, WaveType& wave)
    {
        for (size_t i = 0; i < wave.size(); ++i)
        {
            if (wave.IsCollapsed(i))
                state.m_world.setValue(i, VariableIDMapT::GetValue(wave.GetVariableID(i)));
        }
    }

    static void PropogateInitialValues(SolverState& state, WaveType& wave)
    {
        for (size_t i = 0; i < wave.size(); ++i)
        {
            for (size_t j = 0; j < VariableIDMapT::size(); ++j)
            {
                if (state.m_world.getValue(i) == VariableIDMapT::GetValue(j))
                {
                    CollapseCell(state, wave, static_cast<WorldSizeT>(i), static_cast<VariableIDT>(j));
                    state.m_propagationQueue.push(i);
                    break;
                }
            }
        }
    }
};

} // namespace WFC
