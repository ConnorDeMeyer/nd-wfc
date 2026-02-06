// #pragma once

// #include <array>
// #include <span>
// #include <tuple>

// #include "wfc_bit_container.hpp"
// #include "wfc_utils.hpp"

// namespace WFC {

// template <typename VariableMap, uint8_t Precision>
// struct PrecisionEntry
// {

//     constexpr static uint8_t PrecisionValue = Precision;

//     template <typename MainVariableMap>
//     constexpr static bool UpdatePrecisions(std::span<uint8_t> precisions)
//     {
//         constexpr auto SelectedEntries = VariableMap::GetAllValues();
//         for (auto entry : SelectedEntries)
//         {
//             precisions[MainVariableMap::GetIndex(entry)] = Precision;
//         }
//         return true;
//     }
// };

// enum class EPrecision : uint8_t
// {
//     Precision_0 = 0,
//     Precision_2 = 2,
//     Precision_4 = 4,
//     Precision_8 = 8,
//     Precision_16 = 16,
//     Precision_32 = 32,
//     Precision_64 = 64,
// };

// template <size_t Size, typename AllocatorT, EPrecision ... Precisions>
// class WeightContainers
// {
// private:
//     template <EPrecision Precision>
//     using BitContainerT = BitContainer<static_cast<uint8_t>(Precision), Size, AllocatorT>;
    
//     using TupleT = std::tuple<BitContainerT<Precisions>...>;
//     TupleT m_WeightContainers;

//     static_assert(((static_cast<uint8_t>(Precisions) <= static_cast<uint8_t>(EPrecision::Precision_64)) && ...), "Cannot have precision larger than 64 (double precision)");

// public:
//     WeightContainers() = default;
//     WeightContainers(size_t size)
//         : m_WeightContainers{ BitContainerT<Precisions>(size, AllocatorT()) ... }
//     {}
//     WeightContainers(size_t size, AllocatorT& allocator)
//         : m_WeightContainers{ BitContainerT<Precisions>(size, allocator) ... }
//     {}

// public:
//     static constexpr size_t size()
//     {
//         return sizeof...(Precisions);
//     }

//     /*
//     template <typename ValueT>
//     void SetValue(size_t containerIndex, size_t index, ValueT value)
//     {
//         SetValueFunctions<ValueT>()[containerIndex](*this, index, value);
//     }
//     */
//     void SetValueFloat(size_t containerIndex, size_t index, double value)
//     {
//         SetFloatValueFunctions()[containerIndex](*this, index, value);
//     }

//     template <size_t MaxWeight>
//     uint64_t GetValue(size_t containerIndex, size_t index)
//     {
//         return GetValueFunctions<MaxWeight>()[containerIndex](*this, index);
//     }

// private:
// /*
//     template <typename ValueT>
//     static constexpr auto& SetValueFunctions()
//     {
//         return SetValueFunctions<ValueT>(std::make_index_sequence<size()>());
//     }

//     template <typename ValueT, size_t ... Is>
//     static constexpr auto& SetValueFunctions(std::index_sequence<Is...>)
//     {
//         static constexpr std::array<void(*)(WeightContainers& weightContainers, size_t index, ValueT value), VariableIDMapT::size()> setValueFunctions = 
//         {
//             [] (WeightContainers& weightContainers, size_t index, ValueT value) {
//                 std::get<Is>(weightContainers.m_WeightContainers)[index] = value;
//             },
//             ...
//         };
//         return setValueFunctions;
//     }
// */
//     static constexpr auto& SetFloatValueFunctions()
//     {
//         return SetFloatValueFunctions(std::make_index_sequence<size()>());
//     }

//     template <size_t ... Is>
//     static constexpr auto& SetFloatValueFunctions(std::index_sequence<Is...>)
//     {
//         using FunctionT = void(*)(WeightContainers& weightContainers, size_t index, double value);
//         constexpr std::array<FunctionT, size()> setFloatValueFunctions
//         {
//             [](WeightContainers& weightContainers, size_t index, double value) -> FunctionT {
                
//                 using BitContainerEntryT = typename WeightContainers::TupleT::template tuple_element<Is>::type;
//                 if constexpr (!std::is_same_v<BitContainerEntryT::StorageType, detail::Empty>)
//                 {
//                     constexpr_assert(value >= 0.0 && value <= 1.0, "Value must be between 0.0 and 1.0");
//                     std::get<Is>(weightContainers.m_WeightContainers)[index] = static_cast<BitContainerEntryT::StorageType>(value * BitContainerEntryT::MaxValue);
//                 }
//             }
//             ...
//         };
//         return setFloatValueFunctions;
//     }

//     template <size_t MaxWeight>
//     static constexpr auto& GetValueFunctions()
//     {
//         return GetValueFunctions<MaxWeight>(std::make_index_sequence<size()>());
//     }

//     template <size_t MaxWeight, size_t ... Is>
//     static constexpr auto& GetValueFunctions(std::index_sequence<Is...>)
//     {
//         using FunctionT = uint64_t(*)(WeightContainers& weightContainers, size_t index);
//         constexpr std::array<FunctionT, size()> getValueFunctions = 
//         {
//             [] (WeightContainers& weightContainers, size_t index) -> FunctionT {
//                 using BitContainerEntryT = typename WeightContainers::TupleT::template tuple_element<Is>::type;

//                 if constexpr (std::is_same_v<BitContainerEntryT::StorageType, detail::Empty>)
//                 {
//                     return MaxWeight / 2;
//                 }
//                 else
//                 {
//                     constexpr size_t maxValue = BitContainerEntryT::MaxValue;
//                     if constexpr (maxValue <= MaxWeight)
//                     {
//                         return std::get<Is>(weightContainers.m_WeightContainers)[index];
//                     }
//                     else
//                     {
//                         return static_cast<uint64_t>(std::get<Is>(weightContainers.m_WeightContainers)[index]) * MaxWeight / maxValue;
//                     }
//                 }
//             }
//             ...
//         };
//         return getValueFunctions;
//     }
// };

// /**
// * @brief Compile-time weights storage for weighted random selection
// * @tparam VarT The variable type
// * @tparam VariableIDMapT The variable ID map type
// * @tparam DefaultWeight The default weight for values not explicitly specified
// * @tparam WeightSpecs Variadic template parameters of Weight<VarT, Value, Weight> specifications
// */
// template <typename VariableIDMapT, typename ... PrecisionEntries>
// class WeightsMap {
// public:
//     static constexpr std::array<uint8_t, VariableIDMapT::size()> GeneratePrecisionArray()
//     {
//         std::array<uint8_t, VariableIDMapT::size()> precisionArray{};
        
//         (PrecisionEntries::template UpdatePrecisions<VariableIDMapT>(precisionArray) && ...);

//         return precisionArray;
//     }

//     static constexpr std::array<uint8_t, VariableIDMapT::size()> GetPrecisionArray()
//     {
//         constexpr std::array<uint8_t, VariableIDMapT::size()> precisionArray = GeneratePrecisionArray();
//         return precisionArray;
//     }

//     static constexpr size_t GetPrecision(size_t index)
//     {
//         return GetPrecisionArray()[index];
//     }

//     static constexpr uint8_t GetMaxPrecision()
//     {
//         return std::max<uint8_t>({PrecisionEntries::PrecisionValue ...});
//     }

//     static constexpr uint8_t GetMaxValue()
//     {
//         return (1 << GetMaxPrecision()) - 1;
//     }

//     static constexpr bool HasWeights()
//     {
//         return sizeof...(PrecisionEntries) > 0;
//     }

// public:

//     using VariablesT = VariableIDMapT;

//     template<size_t Size, typename AllocatorT, size_t... Is>
//     auto MakeWeightContainersT(AllocatorT*, std::index_sequence<Is...>)
//     -> WeightContainers<Size, AllocatorT, GetPrecision(Is) ...>;

//     template <size_t Size, typename AllocatorT>
//     using WeightContainersT = decltype(
//         MakeWeightContainersT<Size>(static_cast<AllocatorT*>(nullptr), std::make_index_sequence<VariableIDMapT::size()>{})
//     );

//     template <typename PrecisionEntryT>
//     using Merge = WeightsMap<VariableIDMapT, PrecisionEntries..., PrecisionEntryT>;

//     using WeightT = typename BitContainer<GetMaxPrecision(), 0, std::allocator<uint8_t>>::StorageType;
// };

// }