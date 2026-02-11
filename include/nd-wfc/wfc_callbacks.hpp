#pragma once

namespace WFC {

/**
* @brief Empty callback that accepts any argument
*/
struct EmptyCallback
{
    void operator()(const auto&) const {}
};

/**
 * @brief Callback struct
 * @param CellCollapsedCallbackT The cell collapsed callback type
 * @param ContradictionCallbackT The contradiction callback type
 * @param BranchCallbackT The branch callback type
 */
 template <
 typename CellCollapsedCallbackT = EmptyCallback,
 typename ContradictionCallbackT = EmptyCallback,
 typename BranchCallbackT = EmptyCallback
>
struct Callbacks
{
    using CellCollapsedCallback = CellCollapsedCallbackT;
    using ContradictionCallback = ContradictionCallbackT;
    using BranchCallback = BranchCallbackT;

    template <typename NewCellCollapsedCallbackT>
    using SetCellCollapsedCallbackT = Callbacks<NewCellCollapsedCallbackT, ContradictionCallbackT, BranchCallbackT>;
    template <typename NewContradictionCallbackT>
    using SetContradictionCallbackT = Callbacks<CellCollapsedCallbackT, NewContradictionCallbackT, BranchCallbackT>;
    template <typename NewBranchCallbackT>
    using SetBranchCallbackT = Callbacks<CellCollapsedCallbackT, ContradictionCallbackT, NewBranchCallbackT>;

    static consteval bool HasCellCollapsedCallback() { return !std::is_same_v<CellCollapsedCallbackT, EmptyCallback>; }
    static consteval bool HasContradictionCallback() { return !std::is_same_v<ContradictionCallbackT, EmptyCallback>; }
    static consteval bool HasBranchCallback() { return !std::is_same_v<BranchCallbackT, EmptyCallback>; }
};


}