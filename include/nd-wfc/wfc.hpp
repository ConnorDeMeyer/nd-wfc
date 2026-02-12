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
#include <utility>

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

struct EmptyInitialState {};


template<typename T>
concept WorldType = requires(T world, typename T::ValueType value) {
    { world.size() } -> std::integral;
    { world.setValue(static_cast<decltype(world.size())>(0), value) };
    { world.getValue(static_cast<decltype(world.size())>(0)) } -> std::convertible_to<typename T::ValueType>;
    typename T::ValueType;
};

template <typename T, typename WorldT, typename VarT, typename VariableIDMapT, typename PropagationQueueType, typename UserDataT = std::tuple<>>
concept ConstrainerFunction = requires(T func, const WorldT& world, size_t index, WorldValue<VarT> value, Constrainer<VariableIDMapT, PropagationQueueType, UserDataT>& constrainer) {
    func(world, index, value, constrainer);
};

template <typename WorldT>
concept HasConstexprSize = requires {
    { []() constexpr -> std::size_t { return WorldT{}.size(); }() };
};

// Standalone SolverState struct
template <typename WorldT, typename RandomSelectorT = DefaultRandomSelector<typename WorldT::ValueType>, typename UserDataT = std::tuple<>>
struct SolverState {
    using WorldType = WorldT;
    using WorldSizeT = decltype(WorldT{}.size());
    static constexpr WorldSizeT WorldSize = HasConstexprSize<WorldT> ? WorldT{}.size() : 0;
    using PropagationQueueType = WFCQueue<WorldSize, WorldSizeT>;
    using UserDataType = UserDataT;

    WorldT& m_world;
    PropagationQueueType m_propagationQueue{};
    RandomSelectorT m_randomSelector{};
    WFCStackAllocator m_allocator{};
    size_t m_iterations{};
    UserDataT m_userData{};

    template <typename T>
    T& GetUserData() { return std::get<T>(m_userData); }

    template <typename T>
    const T& GetUserData() const { return std::get<T>(m_userData); }

    SolverState(WorldT& world, uint32_t seed)
        : m_world(world)
        , m_propagationQueue{ WorldSize ? WorldSize : static_cast<WorldSizeT>(world.size()) }
        , m_randomSelector(seed)
    {}

    SolverState(const SolverState& other) = default;
};

// Types-only config struct produced by Builder
template <typename WorldT, typename VarT, typename VariableIDMapT,
          typename ConstrainerFunctionMapT, typename CallbacksT, typename RandomSelectorT,
          typename InitialStateFunctionT = EmptyInitialState,
          typename UserDataT = std::tuple<>>
struct WFCConfig {
    static_assert(WorldType<WorldT>, "WorldT must satisfy World type requirements");

    using WorldSizeT = decltype(WorldT{}.size());
    static constexpr WorldSizeT WorldSize = HasConstexprSize<WorldT> ? WorldT{}.size() : 0;
    using SolverStateType = SolverState<WorldT, RandomSelectorT, UserDataT>;
    using WaveType = Wave<VariableIDMapT, WorldSize>;
    using CallbacksType = CallbacksT;
    using ConstrainerFunctionMapType = ConstrainerFunctionMapT;
    using InitialStateFunctionType = InitialStateFunctionT;
    using UserDataType = UserDataT;
    static consteval bool HasInitialState() { return !std::is_same_v<InitialStateFunctionT, EmptyInitialState>; }
};

// Forward declarations for mutually recursive functions
template <typename CallbacksT, typename ConstrainerFunctionMapT, typename StateT, typename WaveT>
bool RunLoop(StateT& state, WaveT& wave);

template <typename CallbacksT, typename ConstrainerFunctionMapT, typename StateT, typename WaveT>
bool Branch(StateT& state, WaveT& wave);

namespace detail {

template <typename StateT, typename WaveT>
void PopulateWorld(StateT& state, WaveT& wave)
{
    using VariableIDMapT = typename WaveT::IDMapT;
    for (size_t i = 0; i < wave.size(); ++i)
    {
        if (wave.IsCollapsed(i))
            state.m_world.setValue(i, VariableIDMapT::GetValue(wave.GetVariableID(i)));
    }
}

template <typename CallbacksT, typename StateT, typename WaveT>
void CollapseCell(StateT& state, WaveT& wave, typename StateT::WorldSizeT cellId, typename WaveT::VariableIDT value)
{
    using MaskType = typename WaveT::ElementT;
    constexpr_assert(!wave.IsCollapsed(cellId) || wave.GetMask(cellId) == (MaskType(1) << value));
    wave.Collapse(cellId, 1 << value);
    constexpr_assert(wave.IsCollapsed(cellId));

    if constexpr (CallbacksT::HasCellCollapsedCallback())
    {
        PopulateWorld(state, wave);
        typename CallbacksT::CellCollapsedCallback{}(std::as_const(state));
    }
}

template <typename CallbacksT, typename StateT, typename WaveT>
void PropogateInitialValues(StateT& state, WaveT& wave)
{
    using VariableIDMapT = typename WaveT::IDMapT;
    using WorldSizeT = typename StateT::WorldSizeT;
    using VariableIDT = typename WaveT::VariableIDT;
    for (size_t i = 0; i < wave.size(); ++i)
    {
        for (size_t j = 0; j < VariableIDMapT::size(); ++j)
        {
            if (state.m_world.getValue(i) == VariableIDMapT::GetValue(j))
            {
                CollapseCell<CallbacksT>(state, wave, static_cast<WorldSizeT>(i), static_cast<VariableIDT>(j));
                state.m_propagationQueue.push(i);
                break;
            }
        }
    }
}

template <typename ConstrainerFunctionMapT, typename StateT, typename WaveT>
bool Propagate(StateT& state, WaveT& wave)
{
    using VariableIDMapT = typename WaveT::IDMapT;
    using VarT = typename VariableIDMapT::Type;
    using WorldSizeT = typename StateT::WorldSizeT;
    using VariableIDT = typename WaveT::VariableIDT;
    using PropagationQueueType = typename StateT::PropagationQueueType;
    using ConstrainerType = Constrainer<WaveT, PropagationQueueType, typename StateT::UserDataType>;

    while (!state.m_propagationQueue.empty())
    {
        WorldSizeT cellId = state.m_propagationQueue.pop();

        if (wave.IsContradicted(cellId)) return false;

        constexpr_assert(wave.IsCollapsed(cellId), "Cell was not collapsed");

        VariableIDT variableID = wave.GetVariableID(cellId);
        ConstrainerType constrainer(wave, state.m_propagationQueue, state.m_userData);

        using WorldT = typename StateT::WorldType;
        using ConstrainerFunctionPtrT = void(*)(const WorldT&, size_t, WorldValue<VarT>, ConstrainerType&);

        ConstrainerFunctionMapT::template GetFunction<ConstrainerFunctionPtrT>(variableID)(state.m_world, cellId, WorldValue<VarT>{VariableIDMapT::GetValue(variableID), variableID}, constrainer);
    }
    return true;
}

} // namespace detail

template <typename CallbacksT, typename ConstrainerFunctionMapT, typename StateT, typename WaveT>
bool Branch(StateT& state, WaveT& wave)
{
    using VariableIDMapT = typename WaveT::IDMapT;
    using MaskType = typename WaveT::ElementT;
    using WorldSizeT = typename StateT::WorldSizeT;
    using VariableIDT = typename WaveT::VariableIDT;

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
            detail::CollapseCell<CallbacksT>(state, newWave, minEntropyCell, selectedValue);
            state.m_propagationQueue.push(minEntropyCell);

            if (RunLoop<CallbacksT, ConstrainerFunctionMapT>(state, newWave))
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

template <typename CallbacksT, typename ConstrainerFunctionMapT, typename StateT, typename WaveT>
bool RunLoop(StateT& state, WaveT& wave)
{
    static constexpr size_t MaxIterations = 1024 * 16;
    for (; state.m_iterations < MaxIterations; ++state.m_iterations)
    {
        if (!detail::Propagate<ConstrainerFunctionMapT>(state, wave))
            return false;

        if (wave.HasContradiction())
        {
            if constexpr (CallbacksT::HasContradictionCallback())
            {
                detail::PopulateWorld(state, wave);
                typename CallbacksT::ContradictionCallback{}(std::as_const(state));
            }
            return false;
        }

        if (wave.IsFullyCollapsed())
            return true;

        if constexpr (CallbacksT::HasBranchCallback())
        {
            detail::PopulateWorld(state, wave);
            typename CallbacksT::BranchCallback{}(std::as_const(state));
        }

        if (Branch<CallbacksT, ConstrainerFunctionMapT>(state, wave))
            return true;
    }
    return false;
}

template <typename ConfigT>
bool Run(typename ConfigT::SolverStateType& state)
{
    using CallbacksT = typename ConfigT::CallbacksType;
    using ConstrainerFunctionMapT = typename ConfigT::ConstrainerFunctionMapType;
    using WaveType = typename ConfigT::WaveType;
    using VariableIDMapT = typename WaveType::IDMapT;

    WaveType wave{ ConfigT::WorldSize, VariableIDMapT::size(), state.m_allocator };

    detail::PropogateInitialValues<CallbacksT>(state, wave);

    if constexpr (ConfigT::HasInitialState())
    {
        using ConstrainerType = Constrainer<WaveType, typename ConfigT::SolverStateType::PropagationQueueType, typename ConfigT::UserDataType>;
        ConstrainerType constrainer(wave, state.m_propagationQueue, state.m_userData);
        typename ConfigT::InitialStateFunctionType{}(state.m_world, constrainer, state.m_randomSelector);
    }

    if (RunLoop<CallbacksT, ConstrainerFunctionMapT>(state, wave)) {
        detail::PopulateWorld(state, wave);
        return true;
    }
    return false;
}

template <typename ConfigT, typename WorldT, typename... UserDataArgs>
bool Run(WorldT& world, uint32_t seed = std::random_device{}(), UserDataArgs&&... userData)
{
    typename ConfigT::SolverStateType state{ world, seed };
    ((std::get<std::decay_t<UserDataArgs>>(state.m_userData) = std::forward<UserDataArgs>(userData)), ...);
    return Run<ConfigT>(state);
}

template <typename WaveT>
std::optional<typename WaveT::IDMapT::Type> GetValue(WaveT& wave, int cellId) {
    using VariableIDMapT = typename WaveT::IDMapT;
    if (wave.IsCollapsed(cellId)) {
        auto variableId = wave.GetVariableID(cellId);
        return VariableIDMapT::GetValue(variableId);
    }
    return std::nullopt;
}

template <typename ConstrainerFunctionMapT, typename WaveT>
const std::vector<typename WaveT::IDMapT::Type> GetPossibleValues(WaveT& wave, int cellId)
{
    using VariableIDMapT = typename WaveT::IDMapT;
    using VarT = typename VariableIDMapT::Type;
    using MaskType = typename WaveT::ElementT;
    std::vector<VarT> possibleValues;
    MaskType mask = wave.GetMask(cellId);
    for (size_t i = 0; i < ConstrainerFunctionMapT::size(); ++i) {
        if (mask & (1 << i)) possibleValues.push_back(VariableIDMapT::GetValue(i));
    }
    return possibleValues;
}

} // namespace WFC
