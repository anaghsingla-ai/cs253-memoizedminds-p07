#pragma once
#include "cs253/types.hpp"
namespace cs253 {
class RangeStructure {
public:
    virtual ~RangeStructure() = default;
    virtual void build(const std::vector<Value>& initial, Mode mode) = 0;
    virtual std::size_t size() const = 0;
    virtual Mode mode() const = 0;
    virtual void update(Range range, AffineTag tag) = 0;

    virtual Aggregate query(Range range) = 0;
    virtual Counters counters() const noexcept = 0;
    virtual void reset_counters() noexcept = 0;

    virtual InvariantReport check_invariants() const = 0;
};
}
