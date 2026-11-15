
#pragma once

#include "cs253/range_structure.hpp"
#include <vector>

namespace cs253 {

struct InvariantAccess;

class LazySegmentTree final : public RangeStructure {
private:
    struct Node {
        Residue sum = 0;
        Value mn = 0, mx = 0;
        AffineTag lazy = {1, 0};
    };

    std::vector<Node> tree;
    std::size_t n = 0;
    Mode current_mode = Mode::exact_extrema;
    Counters stats{};

    void build_tree(int v, int tl, int tr,
                    const std::vector<Value>& a);

    void apply(int v, int tl, int tr, AffineTag tag);
    void push(int v, int tl, int tr);
    void pull(int v);

    void update_tree(int v, int tl, int tr,
                     int l, int r, AffineTag tag);

    Aggregate query_tree(int v, int tl, int tr,
                         int l, int r);

public:
    LazySegmentTree() = default;
    ~LazySegmentTree() override = default;

    void build(const std::vector<Value>& initial,
               Mode mode) override;

    std::size_t size() const noexcept override;
    Mode mode() const noexcept override;

    void update(Range range, AffineTag tag) override;
    Aggregate query(Range range) override;

    Counters counters() const noexcept override;
    void reset_counters() noexcept override;

    InvariantReport check_invariants() const override;

    friend struct InvariantAccess;
};

}
