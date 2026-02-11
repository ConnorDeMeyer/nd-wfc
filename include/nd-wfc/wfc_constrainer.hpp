#pragma once

#include "wfc_variable_map.hpp"
#include "wfc_queue.hpp"

namespace WFC {

template <typename WorldT, typename WorldSizeT, typename VarT, typename ConstainerType>
struct EmptyConstrainerFunction
{
    static void invoke(const WorldT&, WorldSizeT, WorldValue<VarT>, ConstainerType&) {}
    void operator()(const WorldT&, WorldSizeT, WorldValue<VarT>, ConstainerType&) const {}

    using FuncPtrType = void(*)(const WorldT&, WorldSizeT, WorldValue<VarT>, ConstainerType&);
    operator FuncPtrType() const { return &invoke; }
};

template <typename ... ConstrainerFunctions>
struct ConstrainerFunctionMap {
public:
    static consteval size_t size() { return sizeof...(ConstrainerFunctions); }

    using TupleType = std::tuple<ConstrainerFunctions...>;

    template <typename ConstrainerFunctionPtrT>
    static ConstrainerFunctionPtrT GetFunction(size_t index)
    {
        static_assert((std::is_empty_v<ConstrainerFunctions> && ...), "Lambdas must not have any captures");
        static ConstrainerFunctionPtrT functions[] = {
            static_cast<ConstrainerFunctionPtrT>(ConstrainerFunctions{}) ...
        };
        return functions[index];
    }
};

// Helper to select the correct constrainer function based on the index and the value
template<std::size_t I, 
    typename VariableIDMapT, 
    typename ConstrainerFunctionMapT, 
    typename NewConstrainerFunctionT, 
    typename SelectedIDsVariableIDMapT,
    typename EmptyFunctionT>
using MergedConstrainerElementSelector = 
    std::conditional_t<SelectedIDsVariableIDMapT::template HasValue<VariableIDMapT::GetValue(I)>(), // if the value is in the selected IDs
        NewConstrainerFunctionT,
        std::conditional_t<(I < ConstrainerFunctionMapT::size()), // if the index is within the size of the tuple
                std::tuple_element_t<std::min(I, ConstrainerFunctionMapT::size() - 1), typename ConstrainerFunctionMapT::TupleType>,
                EmptyFunctionT
            >
    >;

// Helper to make a merged constrainer function map
template<typename VariableIDMapT, 
    typename ConstrainerFunctionMapT, 
    typename NewConstrainerFunctionT, 
    typename SelectedIDsVariableIDMapT,
    typename EmptyFunctionT,
    std::size_t... Is>
auto MakeMergedConstrainerIDMap(std::index_sequence<Is...>,VariableIDMapT*, ConstrainerFunctionMapT*, NewConstrainerFunctionT*, SelectedIDsVariableIDMapT*, EmptyFunctionT*)
    -> ConstrainerFunctionMap<MergedConstrainerElementSelector<Is, VariableIDMapT, ConstrainerFunctionMapT, NewConstrainerFunctionT, SelectedIDsVariableIDMapT, EmptyFunctionT>...>;

// Main alias for the merged constrainer function map
template<typename VariableIDMapT, 
    typename ConstrainerFunctionMapT, 
    typename NewConstrainerFunctionT, 
    typename SelectedIDsVariableIDMapT,
    typename EmptyFunctionT>
using MergedConstrainerFunctionMap = decltype(
    MakeMergedConstrainerIDMap(std::make_index_sequence<VariableIDMapT::size()>{}, (VariableIDMapT*)nullptr, (ConstrainerFunctionMapT*)nullptr, (NewConstrainerFunctionT*)nullptr, (SelectedIDsVariableIDMapT*)nullptr, (EmptyFunctionT*)nullptr)
);

/**
 * @brief Constrainer class used in constraint functions to limit possible values for other cells
 */
template <typename WaveT, typename PropagationQueueT>
class Constrainer {
public:
    using IDMapT = typename WaveT::IDMapT;
    using BitContainerT = typename WaveT::BitContainerT;
    using MaskType = typename BitContainerT::StorageType;

public:
    Constrainer(WaveT& wave, PropagationQueueT& propagationQueue)
        : m_wave(wave)
        , m_propagationQueue(propagationQueue)
    {}

    /**
        * @brief Constrain a cell to exclude specific values
        * @param cellId The ID of the cell to constrain
        * @param forbiddenValues The set of forbidden values for this cell
        */
    template <typename IDMapT::Type ... ExcludedValues>
    void Exclude(size_t cellId) {
        static_assert(sizeof...(ExcludedValues) > 0, "At least one excluded value must be provided");
        auto indices = IDMapT::template ValuesToIndices<ExcludedValues...>();
        ApplyMask(cellId, ~BitContainerT::GetMask(indices));
    }

    void Exclude(WorldValue<typename IDMapT::Type> value, size_t cellId) {
        ApplyMask(cellId, ~(1 << value.InternalIndex));
    }

    /**
        * @brief Constrain a cell to only allow one specific value
        * @param cellId The ID of the cell to constrain
        * @param value The only allowed value for this cell
        */
    template <typename IDMapT::Type ... AllowedValues>
    void Only(size_t cellId) {
        static_assert(sizeof...(AllowedValues) > 0, "At least one allowed value must be provided");
        auto indices = IDMapT::template ValuesToIndices<AllowedValues...>();
        ApplyMask(cellId, BitContainerT::GetMask(indices));
    }

    void Only(WorldValue<typename IDMapT::Type> value, size_t cellId) {
        ApplyMask(cellId, 1 << value.InternalIndex);
    }

    /**
        * @brief Re-enable specific values for a cell (OR bits back in)
        * @param cellId The ID of the cell to modify
        */
    template <typename IDMapT::Type ... IncludedValues>
    void Include(size_t cellId) {
        static_assert(sizeof...(IncludedValues) > 0, "At least one included value must be provided");
        if (m_wave.IsCollapsed(cellId)) return; // don't un-collapse decided cells
        auto indices = IDMapT::template ValuesToIndices<IncludedValues...>();
        m_wave.Enable(cellId, BitContainerT::GetMask(indices));
    }

    void Include(WorldValue<typename IDMapT::Type> value, size_t cellId) {
        if (m_wave.IsCollapsed(cellId)) return;
        m_wave.Enable(cellId, 1 << value.InternalIndex);
    }

private:
    void ApplyMask(size_t cellId, MaskType mask) {
        bool wasCollapsed = m_wave.IsCollapsed(cellId);

        m_wave.Collapse(cellId, mask);

        bool collapsed = m_wave.IsCollapsed(cellId);
        if (!wasCollapsed && collapsed) {
            m_propagationQueue.push(cellId);
        }
    }

private:
    WaveT& m_wave;
    PropagationQueueT& m_propagationQueue;
};

}