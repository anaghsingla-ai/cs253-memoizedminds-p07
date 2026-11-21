#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/array_oracle.hpp"
#include "cs253/pipeline.hpp"
#include <type_traits>
#include <stdexcept>
#include <typeinfo>
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
        catch (const std::logic_error& error) {
            return typeid(error) == typeid(std::logic_error);
        }
        catch (...) { return false; }
        return false;
    };
    if (!throws_logic_error([&] { (void)tree.size(); }) ||
        !throws_logic_error([&] { (void)tree.mode(); }) ||
        !throws_logic_error([&] { (void)blocks.size(); }) ||
        !throws_logic_error([&] { (void)blocks.mode(); })) return 1;
    for (const cs253::Range range : {cs253::Range{0, 0}, cs253::Range{0, 1}}) {
        if (!throws_logic_error([&] { tree.update(range, {1, 0}); }) ||
            !throws_logic_error([&] { (void)tree.query(range); })) return 1;
    }
    auto rejects_invalid_mode = [&] {
        try { tree.build({}, static_cast<cs253::Mode>(99)); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    };
    if (!rejects_invalid_mode() ||
        !throws_logic_error([&] { (void)tree.size(); })) return 1;
    tree.build({}, cs253::Mode::modular_sum);
    tree.update({0, 0}, {1, 0});
    if (tree.query({0, 0}).length != 0) return 1;
    const std::vector<cs253::Value> initial{4, 7, 2};
    tree.build(initial, cs253::Mode::exact_extrema);
    blocks.build(initial, cs253::Mode::exact_extrema);
    if (!rejects_invalid_mode() || tree.query({0, 3}).sum_mod != 13) return 1;
    if (tree.size() != initial.size() || blocks.size() != initial.size() ||
        tree.mode() != cs253::Mode::exact_extrema ||
        blocks.mode() != cs253::Mode::exact_extrema) return 1;
    const cs253::AffineTag identity;
    const cs253::Aggregate empty;
    return identity.a == 1 && identity.b == 0 && empty.length == 0 &&
           empty.sum_mod == 0 && !empty.minimum && !empty.maximum ? 0 : 1;
}
