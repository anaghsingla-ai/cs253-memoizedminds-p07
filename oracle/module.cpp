#include <string>
#include "cs253/array_oracle.hpp"
#include "cs253/pipeline.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace cs253 {

namespace {

using Wide = __int128_t;

// Normalize any integer into [0,modulus).
Residue norm(Wide x) {
    x%=modulus;
    if(x<0)x+=modulus;
    return static_cast<Residue>(x);
}

// Convert to int64 only if it is within range.
Value checked(Wide x) {
    if(x<std::numeric_limits<Value>::min() ||
       x>std::numeric_limits<Value>::max()) {
        throw std::overflow_error("ArrayOracle: integer overflow");
    }
    return static_cast<Value>(x);
}

}

// Actual implementation hidden behind PIMPL.
struct ArrayOracle::Impl {
    std::vector<Value> arr;
    Mode current_mode=Mode::exact_extrema;
    bool built=false;
    Counters stats{};
};

ArrayOracle::ArrayOracle()
    : impl_(std::make_unique<Impl>()) {}

ArrayOracle::~ArrayOracle()=default;


// BUILD
void ArrayOracle::build(
    const std::vector<Value>& initial,
    Mode requested_mode
) {
    std::vector<Value> temp;
    temp.reserve(initial.size());

    for(Value x:initial) {
        if(requested_mode==Mode::modular_sum)
            temp.push_back(norm(x));
        else
            temp.push_back(x);
    }

    impl_->arr=std::move(temp);
    impl_->current_mode=requested_mode;
    impl_->stats={};
    impl_->built=true;
}


// SIZE
std::size_t ArrayOracle::size() const noexcept {
    return impl_->arr.size();
}


// MODE
Mode ArrayOracle::mode() const noexcept {
    return impl_->current_mode;
}


// UPDATE
void ArrayOracle::update(
    Range range,
    AffineTag tag
) {
    if(!impl_->built)
        throw std::logic_error("ArrayOracle: build() not called");

    std::size_t n=impl_->arr.size();

    // Validate range before modifying anything.
    if(range.begin>range.end || range.end>n)
        throw std::out_of_range("ArrayOracle: invalid range");

    bool exact=(impl_->current_mode==Mode::exact_extrema);

    // Exact mode allows only -1, 0, 1 as multiplier.
    if(exact && tag.a!=-1 && tag.a!=0 && tag.a!=1)
        throw std::invalid_argument("ArrayOracle: invalid exact gain");

    // First pass: validate all exact results.
    // This ensures an overflow cannot partially modify the array.
    if(exact) {
        for(std::size_t i=range.begin;i<range.end;i++) {
            checked(Wide(tag.a)*impl_->arr[i]+tag.b);
        }
    }

    // Second pass: actually perform the update.
    for(std::size_t i=range.begin;i<range.end;i++) {
        if(exact) {
            impl_->arr[i]=checked(
                Wide(tag.a)*impl_->arr[i]+tag.b
            );
        }
        else {
            impl_->arr[i]=norm(
                Wide(norm(tag.a))*impl_->arr[i]+norm(tag.b)
            );
        }
    }
}


// QUERY
Aggregate ArrayOracle::query(Range range) {
    if(!impl_->built)
        throw std::logic_error("ArrayOracle: build() not called");

    std::size_t n=impl_->arr.size();

    if(range.begin>range.end || range.end>n)
        throw std::out_of_range("ArrayOracle: invalid range");

    Aggregate result;
    result.length=range.end-range.begin;

    if(result.length==0)
        return result;

    Wide sum=0;

    Value mn=impl_->arr[range.begin];
    Value mx=impl_->arr[range.begin];

    for(std::size_t i=range.begin;i<range.end;i++) {
        Value x=impl_->arr[i];

        sum+=x;

        if(x<mn)mn=x;
        if(x>mx)mx=x;
    }

    result.sum_mod=norm(sum);

    if(impl_->current_mode==Mode::exact_extrema) {
        result.minimum=mn;
        result.maximum=mx;
    }

    return result;
}


// COUNTERS
Counters ArrayOracle::counters() const noexcept {
    return impl_->stats;
}


// RESET COUNTERS
void ArrayOracle::reset_counters() noexcept {
    impl_->stats={};
}


// INVARIANT CHECKER
InvariantReport ArrayOracle::check_invariants() const {
    return InvariantAccess::check(*this);
}


// Oracle invariant access.
InvariantReport InvariantAccess::check(const ArrayOracle& oracle) {
    InvariantReport report{true,{}};

    const auto& p=*oracle.impl_;

    auto fail=[&](const std::string& message) {
        report.ok=false;
        report.violations.push_back(message);
    };

    if(!p.built) {
        fail("ArrayOracle has not been built");
        return report;
    }

    for(std::size_t i=0;i<p.arr.size();i++) {
        Value x=p.arr[i];

        if(p.current_mode==Mode::modular_sum) {
            if(x<0 || x>=modulus)
                fail("Non-canonical modular value at index "+
                     std::to_string(i));
        }
    }

    return report;
}

} // namespace cs253
