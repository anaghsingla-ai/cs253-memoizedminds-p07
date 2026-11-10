#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/array_oracle.hpp"
#include "cs253/pipeline.hpp"
#include <type_traits>
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::LazySegmentTree>);
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::SqrtDecomposition>);
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::ArrayOracle>);
static_assert(!std::is_abstract_v<cs253::LazySegmentTree>);
static_assert(!std::is_abstract_v<cs253::SqrtDecomposition>);
static_assert(!std::is_abstract_v<cs253::ArrayOracle>);
int main() {
    const cs253::AffineTag identity;
    const cs253::Aggregate empty;
    return identity.a == 1 && identity.b == 0 && empty.length == 0 &&
           empty.sum_mod == 0 && !empty.minimum && !empty.maximum ? 0 : 1;
}
