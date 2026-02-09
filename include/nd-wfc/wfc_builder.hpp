#pragma once

namespace WFC {

#include "wfc_utils.hpp"
#include "wfc_variable_map.hpp"
#include "wfc_constrainer.hpp"
#include "wfc_callbacks.hpp"
#include "wfc_random.hpp"
#include "wfc.hpp"

/**
* @brief Builder class for creating WFC instances
*/
template<
    typename WorldT, 
    typename VarT = typename WorldT::ValueType, 
    typename VariableIDMapT = VariableIDMap<VarT>, 
    typename ConstrainerFunctionMapT = ConstrainerFunctionMap<void*>, 
    typename CallbacksT = Callbacks<WorldT>, 
    typename RandomSelectorT = DefaultRandomSelector<VarT>,
    typename SelectedValueT = void>
class Builder {
public:
    using WorldSizeT = decltype(WorldT{}.size());
    constexpr static WorldSizeT WorldSize = HasConstexprSize<WorldT> ? WorldT{}.size() : 0;

    using WaveType = Wave<VariableIDMapT, WorldSize>;
    using PropagationQueueType = WFCQueue<WorldSize, WorldSizeT>;
    using ConstrainerType = Constrainer<WaveType, PropagationQueueType>;


    template <VarT ... Values>
    using DefineIDs = Builder<WorldT, VarT, VariableIDMap<VarT, Values...>, ConstrainerFunctionMapT, CallbacksT, RandomSelectorT, VariableIDMap<VarT, Values...>>;

    template <size_t RangeStart, size_t RangeEnd>
    using DefineRange = Builder<WorldT, VarT, VariableIDRange<VarT, RangeStart, RangeEnd>, ConstrainerFunctionMapT, CallbacksT, RandomSelectorT, VariableIDRange<VarT, RangeStart, RangeEnd>>;

    template <size_t RangeEnd>
    using DefineRange0 = Builder<WorldT, VarT, VariableIDRange<VarT, 0, RangeEnd>, ConstrainerFunctionMapT, CallbacksT, RandomSelectorT, VariableIDRange<VarT, 0, RangeEnd>>;


    template <VarT ... Values>
    using Variable = Builder<WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT, CallbacksT, RandomSelectorT, VariableIDMap<VarT, Values...>>;

    template <size_t RangeStart, size_t RangeEnd>
    using VariableRange = Builder<WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT, CallbacksT, RandomSelectorT, VariableIDRange<VarT, RangeStart, RangeEnd>>;

    
    using EmptyConstrainerFunctionT = EmptyConstrainerFunction<WorldT, WorldSizeT, VarT, ConstrainerType>;

    template <typename ConstrainerFunctionT>
        requires ConstrainerFunction<ConstrainerFunctionT, WorldT, VarT, WaveType, PropagationQueueType>
    using Constrain = Builder<WorldT, VarT, VariableIDMapT,
        MergedConstrainerFunctionMap<
            VariableIDMapT,
            ConstrainerFunctionMapT,
            ConstrainerFunctionT,
            SelectedValueT,
            EmptyConstrainerFunctionT
            >, CallbacksT, RandomSelectorT, SelectedValueT
        >;

    template <typename ConstrainerFunctionT>
    requires ConstrainerFunction<ConstrainerFunctionT, WorldT, VarT, WaveType, PropagationQueueType>
    using ConstrainAll = Builder<WorldT, VarT, VariableIDMapT,
        MergedConstrainerFunctionMap<
            VariableIDMapT,
            ConstrainerFunctionMapT,
            ConstrainerFunctionT,
            VariableIDMapT,
            EmptyConstrainerFunctionT
            >, CallbacksT, RandomSelectorT
        >;

        
    template <typename NewCellCollapsedCallbackT>
    using SetCellCollapsedCallback = Builder<WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT, typename CallbacksT::template SetCellCollapsedCallbackT<NewCellCollapsedCallbackT>, RandomSelectorT>;
    template <typename NewContradictionCallbackT>
    using SetContradictionCallback = Builder<WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT, typename CallbacksT::template SetContradictionCallbackT<NewContradictionCallbackT>, RandomSelectorT>;
    template <typename NewBranchCallbackT>
    using SetBranchCallback = Builder<WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT, typename CallbacksT::template SetBranchCallbackT<NewBranchCallbackT>, RandomSelectorT>;


    template <typename NewRandomSelectorT>
    using SetRandomSelector = Builder<WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT, CallbacksT, NewRandomSelectorT>;

    using Build = WFCConfig<WorldT, VarT, VariableIDMapT, ConstrainerFunctionMapT, CallbacksT, RandomSelectorT>;
};

}