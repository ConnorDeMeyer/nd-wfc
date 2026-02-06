#pragma once

namespace WFC {

/**
* @brief Empty callback function
* @param WorldT The world type
*/
template <typename WorldT>
struct EmptyCallback
{
    void operator()(WorldT&) const {}
};

/**
 * @brief Callback struct
 * @param WorldT The world type
 * @param AllCellsCollapsedCallbackT The all cells collapsed callback type
 * @param CellCollapsedCallbackT The cell collapsed callback type
 * @param ContradictionCallbackT The contradiction callback type
 * @param BranchCallbackT The branch callback type
 */
 template <typename WorldT, 
 typename CellCollapsedCallbackT = EmptyCallback<WorldT>,
 typename ContradictionCallbackT = EmptyCallback<WorldT>,
 typename BranchCallbackT = EmptyCallback<WorldT>
>
struct Callbacks
{
    using CellCollapsedCallback = CellCollapsedCallbackT;
    using ContradictionCallback = ContradictionCallbackT;
    using BranchCallback = BranchCallbackT;

    template <typename NewCellCollapsedCallbackT>
    using SetCellCollapsedCallbackT = Callbacks<WorldT, NewCellCollapsedCallbackT, ContradictionCallbackT, BranchCallbackT>;
    template <typename NewContradictionCallbackT>
    using SetContradictionCallbackT = Callbacks<WorldT, CellCollapsedCallbackT, NewContradictionCallbackT, BranchCallbackT>;
    template <typename NewBranchCallbackT>
    using SetBranchCallbackT = Callbacks<WorldT, CellCollapsedCallbackT, ContradictionCallbackT, NewBranchCallbackT>;

    static consteval bool HasCellCollapsedCallback() { return !std::is_same_v<CellCollapsedCallbackT, EmptyCallback<WorldT>>; }
    static consteval bool HasContradictionCallback() { return !std::is_same_v<ContradictionCallbackT, EmptyCallback<WorldT>>; }
    static consteval bool HasBranchCallback() { return !std::is_same_v<BranchCallbackT, EmptyCallback<WorldT>>; }
};


}