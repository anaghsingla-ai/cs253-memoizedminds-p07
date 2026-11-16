#pragma once
#include "cs253/range_structure.hpp"
#include <memory>
namespace cs253 {
struct InvariantAccess;

class ArrayOracle final : public RangeStructure {
public:
    ArrayOracle();
    ~ArrayOracle() override;
    ArrayOracle(const ArrayOracle&) = delete;
    ArrayOracle& operator=(const ArrayOracle&) = delete;
    void build(const std::vector<Value>& initial, Mode mode) override;
    std::size_t size() const noexcept override;
    Mode mode() const noexcept override;
    void update(Range range, AffineTag tag) override;
    Aggregate query(Range range) override;
    Counters counters() const noexcept override;
    void reset_counters() noexcept override;
    InvariantReport check_invariants() const override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend struct InvariantAccess;
};
}
