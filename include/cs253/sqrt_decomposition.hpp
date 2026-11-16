#pragma once
#include "cs253/range_structure.hpp"
#include <memory>
namespace cs253 {
struct InvariantAccess;

class SqrtDecomposition final : public RangeStructure {
public:
    SqrtDecomposition();
    ~SqrtDecomposition() override;
    SqrtDecomposition(const SqrtDecomposition&) = delete;
    SqrtDecomposition& operator=(const SqrtDecomposition&) = delete;
    void build(const std::vector<Value>& initial, Mode mode) override;
    std::size_t size() const override;
    Mode mode() const override;
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
