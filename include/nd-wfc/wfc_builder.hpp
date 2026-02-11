#pragma once

namespace WFC {

#include "wfc_utils.hpp"
#include "wfc_variable_map.hpp"
#include "wfc_constrainer.hpp"
#include "wfc_callbacks.hpp"
#include "wfc_random.hpp"
#include "wfc.hpp"

namespace detail {

template<typename T, typename Tuple>
struct TupleContains : std::false_type {};

template<typename T, typename U, typename... Rest>
struct TupleContains<T, std::tuple<U, Rest...>> : TupleContains<T, std::tuple<Rest...>> {};

template<typename T, typename... Rest>
struct TupleContains<T, std::tuple<T, Rest...>> : std::true_type {};

template<typename T, typename Tuple>
struct TupleAppendUnique;

template<typename T, typename... Ts>
struct TupleAppendUnique<T, std::tuple<Ts...>> {
    static_assert(!TupleContains<T, std::tuple<Ts...>>::value, "AddUserData: type already exists in UserData tuple (all types must be unique)");
    using type = std::tuple<Ts..., T>;
};

} // namespace detail

/**
* @brief Groups core WFC types: World, VarT, VariableIDMap, ConstrainerFunctionMap
*/
template<typename WorldT_, typename VarT_, typename VariableIDMapT_, typename ConstrainerFunctionMapT_>
struct CoreTypes {
    using WorldT = WorldT_;
    using VarT = VarT_;
    using VariableIDMapT = VariableIDMapT_;
    using ConstrainerFunctionMapT = ConstrainerFunctionMapT_;
};

/**
* @brief Groups optional WFC types: Callbacks, RandomSelector, InitialStateFunction, UserData
*/
template<typename CallbacksT_, typename RandomSelectorT_, typename InitialStateFunctionT_, typename UserDataT_ = std::tuple<>>
struct OptionTypes {
    using CallbacksT = CallbacksT_;
    using RandomSelectorT = RandomSelectorT_;
    using InitialStateFunctionT = InitialStateFunctionT_;
    using UserDataT = UserDataT_;
};

/**
* @brief Builder class for creating WFC instances
*
* Template parameters are organized into sub-builders:
*   CoreT:          WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT
*   OptionsT:       CallbacksT, RandomSelectorT, InitialStateFunctionT, UserDataT
*   SelectedValueT: always last, tracks the currently selected variable for Constrain
*/
template<typename CoreT, typename OptionsT, typename SelectedValueT = void>
class BuilderImpl {
public:
    // --- Extracted core types ---
    using WorldT = typename CoreT::WorldT;
    using VarT = typename CoreT::VarT;
    using VariableIDMapT = typename CoreT::VariableIDMapT;
    using ConstrainerFunctionMapT = typename CoreT::ConstrainerFunctionMapT;

    // --- Extracted option types ---
    using CallbacksT = typename OptionsT::CallbacksT;
    using RandomSelectorT = typename OptionsT::RandomSelectorT;
    using InitialStateFunctionT = typename OptionsT::InitialStateFunctionT;
    using UserDataT = typename OptionsT::UserDataT;

    // --- Derived types ---
    using WorldSizeT = decltype(WorldT{}.size());
    constexpr static WorldSizeT WorldSize = HasConstexprSize<WorldT> ? WorldT{}.size() : 0;

    using WaveType = Wave<VariableIDMapT, WorldSize>;
    using PropagationQueueType = WFCQueue<WorldSize, WorldSizeT>;
    using ConstrainerType = Constrainer<WaveType, PropagationQueueType>;


    // === Core: Define variable IDs ===

    template <VarT ... Values>
    using DefineIDs = BuilderImpl<
        CoreTypes<WorldT, VarT, VariableIDMap<VarT, Values...>, ConstrainerFunctionMapT>,
        OptionsT,
        VariableIDMap<VarT, Values...>>;

    template <size_t RangeStart, size_t RangeEnd>
    using DefineRange = BuilderImpl<
        CoreTypes<WorldT, VarT, VariableIDRange<VarT, RangeStart, RangeEnd>, ConstrainerFunctionMapT>,
        OptionsT,
        VariableIDRange<VarT, RangeStart, RangeEnd>>;

    template <size_t RangeEnd>
    using DefineRange0 = BuilderImpl<
        CoreTypes<WorldT, VarT, VariableIDRange<VarT, 0, RangeEnd>, ConstrainerFunctionMapT>,
        OptionsT,
        VariableIDRange<VarT, 0, RangeEnd>>;


    // === Core: Select variables for constraints ===

    template <VarT ... Values>
    using Variable = BuilderImpl<CoreT, OptionsT, VariableIDMap<VarT, Values...>>;

    template <size_t RangeStart, size_t RangeEnd>
    using VariableRange = BuilderImpl<CoreT, OptionsT, VariableIDRange<VarT, RangeStart, RangeEnd>>;


    // === Core: Add constraints ===

    using EmptyConstrainerFunctionT = EmptyConstrainerFunction<WorldT, WorldSizeT, VarT, ConstrainerType>;

    template <typename ConstrainerFunctionT>
        requires ConstrainerFunction<ConstrainerFunctionT, WorldT, VarT, WaveType, PropagationQueueType>
    using Constrain = BuilderImpl<
        CoreTypes<WorldT, VarT, VariableIDMapT,
            MergedConstrainerFunctionMap<
                VariableIDMapT,
                ConstrainerFunctionMapT,
                ConstrainerFunctionT,
                SelectedValueT,
                EmptyConstrainerFunctionT
            >>,
        OptionsT,
        SelectedValueT>;

    template <typename ConstrainerFunctionT>
        requires ConstrainerFunction<ConstrainerFunctionT, WorldT, VarT, WaveType, PropagationQueueType>
    using ConstrainAll = BuilderImpl<
        CoreTypes<WorldT, VarT, VariableIDMapT,
            MergedConstrainerFunctionMap<
                VariableIDMapT,
                ConstrainerFunctionMapT,
                ConstrainerFunctionT,
                VariableIDMapT,
                EmptyConstrainerFunctionT
            >>,
        OptionsT,
        SelectedValueT>;


    // === Options: Callbacks ===

    template <typename NewCellCollapsedCallbackT>
    using SetCellCollapsedCallback = BuilderImpl<
        CoreT,
        OptionTypes<typename CallbacksT::template SetCellCollapsedCallbackT<NewCellCollapsedCallbackT>, RandomSelectorT, InitialStateFunctionT, UserDataT>,
        SelectedValueT>;

    template <typename NewContradictionCallbackT>
    using SetContradictionCallback = BuilderImpl<
        CoreT,
        OptionTypes<typename CallbacksT::template SetContradictionCallbackT<NewContradictionCallbackT>, RandomSelectorT, InitialStateFunctionT, UserDataT>,
        SelectedValueT>;

    template <typename NewBranchCallbackT>
    using SetBranchCallback = BuilderImpl<
        CoreT,
        OptionTypes<typename CallbacksT::template SetBranchCallbackT<NewBranchCallbackT>, RandomSelectorT, InitialStateFunctionT, UserDataT>,
        SelectedValueT>;


    // === Options: Random selector ===

    template <typename NewRandomSelectorT>
    using SetRandomSelector = BuilderImpl<
        CoreT,
        OptionTypes<CallbacksT, NewRandomSelectorT, InitialStateFunctionT, UserDataT>,
        SelectedValueT>;


    // === Options: Initial state ===

    template <typename NewInitialStateFunctionT>
    using SetInitialState = BuilderImpl<
        CoreT,
        OptionTypes<CallbacksT, RandomSelectorT, NewInitialStateFunctionT, UserDataT>,
        SelectedValueT>;


    // === Options: User data ===

    template <typename T>
    using AddUserData = BuilderImpl<
        CoreT,
        OptionTypes<CallbacksT, RandomSelectorT, InitialStateFunctionT,
            typename detail::TupleAppendUnique<T, UserDataT>::type>,
        SelectedValueT>;


    // === Build ===

    using Build = WFCConfig<WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT, CallbacksT, RandomSelectorT, InitialStateFunctionT, UserDataT>;
};

/**
* @brief Convenience alias - preserves the original Builder<WorldT> entry point
*/
template <typename WorldT>
using Builder = BuilderImpl<
    CoreTypes<WorldT, typename WorldT::ValueType, VariableIDMap<typename WorldT::ValueType>, ConstrainerFunctionMap<void*>>,
    OptionTypes<Callbacks<>, DefaultRandomSelector<typename WorldT::ValueType>, EmptyInitialState>
>;

}
