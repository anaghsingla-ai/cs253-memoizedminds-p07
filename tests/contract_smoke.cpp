#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/array_oracle.hpp"
#include "cs253/pipeline.hpp"
#include <type_traits>
#include <stdexcept>
#include <vector>
#include <utility>
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::LazySegmentTree>);
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::SqrtDecomposition>);
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::ArrayOracle>);
static_assert(!std::is_abstract_v<cs253::LazySegmentTree>);
static_assert(!std::is_abstract_v<cs253::SqrtDecomposition>);
static_assert(!std::is_abstract_v<cs253::ArrayOracle>);
static_assert(!noexcept(std::declval<const cs253::RangeStructure&>().size()));
static_assert(!noexcept(std::declval<const cs253::RangeStructure&>().mode()));
static_assert(!noexcept(std::declval<const cs253::LazySegmentTree&>().size()));
static_assert(!noexcept(std::declval<const cs253::LazySegmentTree&>().mode()));
static_assert(!noexcept(std::declval<const cs253::SqrtDecomposition&>().size()));
static_assert(!noexcept(std::declval<const cs253::SqrtDecomposition&>().mode()));
static_assert(!noexcept(std::declval<const cs253::ArrayOracle&>().size()));
static_assert(!noexcept(std::declval<const cs253::ArrayOracle&>().mode()));
int main() {
    cs253::LazySegmentTree tree;
    cs253::SqrtDecomposition blocks;
    auto throws_logic_error = [](auto&& action) {
        try { action(); }
        catch (const std::logic_error&) { return true; }
        catch (...) { return false; }
        return false;
    };
    if (!throws_logic_error([&] { (void)tree.size(); }) ||
        !throws_logic_error([&] { (void)tree.mode(); }) ||
        !throws_logic_error([&] { (void)blocks.size(); }) ||
        !throws_logic_error([&] { (void)blocks.mode(); })) return 1;
    const std::vector<cs253::Value> initial{4, 7, 2};
    tree.build(initial, cs253::Mode::exact_extrema);
    blocks.build(initial, cs253::Mode::exact_extrema);
    if (tree.size() != initial.size() || blocks.size() != initial.size() ||
        tree.mode() != cs253::Mode::exact_extrema ||
        blocks.mode() != cs253::Mode::exact_extrema) return 1;
    const cs253::AffineTag identity;
    const cs253::Aggregate empty;
    return identity.a == 1 && identity.b == 0 && empty.length == 0 &&
           empty.sum_mod == 0 && !empty.minimum && !empty.maximum ? 0 : 1;
}
