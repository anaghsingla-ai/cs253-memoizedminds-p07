// Structure B: square-root decomposition with lazy updates x -> a*x + b.
//
// The array is split into blocks of about sqrt(n) elements. Each block stores:
//   - a pending tag (a, b): every raw value in the block still owes "x -> a*x + b",
//   - the sum, min and max of the block's REAL values, where real value = a*raw + b.
//
// Update of a whole block : change only the tag and the summary  -> O(1)
// Update of part of a block: work directly on the raw values      -> O(block size)
// A range covers at most n/b whole blocks and 2 partial blocks, so every
// update and query costs O(n/b + b) = O(sqrt n) when b is about sqrt n.
//
// More detail: src/structB/DESIGN.md. Proofs: docs/derivations/D2.md.
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/sqrt_config.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace cs253 {

namespace {

// 128-bit integer: big enough for the product of two int64 values, so we can
// compute a*x + b first and then check or reduce the result.
using int128 = __int128_t;

std::size_t chosen_block_size = 0;  // 0 means automatic: ceil(sqrt(n))

// x mod P, always in [0, P) even when x is negative.
Residue mod_p(int128 x) {
    x %= modulus;
    if (x < 0) x += modulus;
    return (Residue)x;
}

// Exact mode must stay inside int64. Throw instead of silently wrapping around.
Value fit_int64(int128 x) {
    if (x < INT64_MIN || x > INT64_MAX)
        throw std::overflow_error("SqrtDecomposition: value does not fit in int64");
    return (Value)x;
}

// Exact mode only allows gains -1, 0 and 1.
bool allowed_exact_gain(Value a) { return a == -1 || a == 0 || a == 1; }

void report_problem(InvariantReport& report, const std::string& message) {
    report.ok = false;
    report.violations.push_back(message);
}

}  // namespace

void set_sqrt_block_size(std::size_t block_size) noexcept { chosen_block_size = block_size; }
std::size_t sqrt_block_size_setting() noexcept { return chosen_block_size; }

struct SqrtDecomposition::Impl {
    struct Block {
        Value a = 1, b = 0;     // pending tag; (1, 0) means nothing is pending
        Residue sum = 0;        // sum of real values, mod P
        Value mn = 0, mx = 0;   // min and max of real values (exact mode only)
    };

    bool built = false;
    Mode mode = Mode::exact_extrema;
    std::size_t n = 0;
    std::size_t block_size = 1;
    std::vector<Value> raw;     // modular mode: stored as residues in [0, P)
    std::vector<Block> blocks;
    Counters counters;

    bool exact() const { return mode == Mode::exact_extrema; }
    std::size_t block_start(std::size_t k) const { return k * block_size; }
    std::size_t block_end(std::size_t k) const { return std::min(n, (k + 1) * block_size); }  // last block may be shorter

    // a*x + b in the arithmetic of the current mode.
    Value apply(Value a, Value b, Value x) const {
        int128 y = (int128)a * x + b;
        return exact() ? fit_int64(y) : mod_p(y);
    }

    // Recompute the sum, min and max of block k from its raw values.
    // Only valid when the block's tag is (1, 0), i.e. raw values are the real values.
    void recompute_block(std::size_t k) {
        Block& blk = blocks[k];
        int128 sum = 0;
        blk.mn = INT64_MAX;
        blk.mx = INT64_MIN;
        for (std::size_t i = block_start(k); i < block_end(k); i++) {
            sum += raw[i];
            blk.mn = std::min(blk.mn, raw[i]);
            blk.mx = std::max(blk.mx, raw[i]);
        }
        blk.sum = mod_p(sum);
    }

    // Apply block k's pending tag to its raw values and clear the tag.
    // The summary stays the same, because it already included the tag.
    void push_tag(std::size_t k) {
        Block& blk = blocks[k];
        if (blk.a == 1 && blk.b == 0) return;  // nothing pending
        for (std::size_t i = block_start(k); i < block_end(k); i++)
            raw[i] = apply(blk.a, blk.b, raw[i]);
        blk.a = 1;
        blk.b = 0;
    }

    void build(const std::vector<Value>& initial, Mode new_mode) {
        // Block size: the user's setting, or the smallest b with b*b >= n.
        std::size_t size = initial.size();
        std::size_t b = chosen_block_size;
        if (b == 0) {
            b = (std::size_t)std::sqrt((double)size);
            while (b * b < size) b++;  // fixes floating-point rounding
        }
        if (b == 0) b = 1;  // empty array

        // Fill new vectors first, so if memory runs out the old data is kept.
        std::vector<Value> new_raw = initial;
        if (new_mode == Mode::modular_sum)
            for (Value& v : new_raw) v = mod_p(v);
        std::vector<Block> new_blocks((size + b - 1) / b);

        raw.swap(new_raw);
        blocks.swap(new_blocks);
        n = size;
        block_size = b;
        mode = new_mode;
        for (std::size_t k = 0; k < blocks.size(); k++) recompute_block(k);
        counters = Counters();
        built = true;
    }

    void check_built_and_range(Range r, const std::string& operation) const {
        if (!built) throw std::logic_error("SqrtDecomposition::" + operation + ": build() has not been called");
        if (r.begin > r.end || r.end > n) throw std::out_of_range("SqrtDecomposition::" + operation + ": invalid range");
    }

    // The whole of block k is updated: O(1). Raw values are not touched.
    void full_update(std::size_t k, Value a, Value b) {
        counters.full_blocks++;
        counters.tag_compositions++;
        Block& blk = blocks[k];
        std::size_t len = block_end(k) - block_start(k);

        // New summary. Every real value x becomes a*x + b, so
        //   sum      -> a*sum + b*len
        //   min, max -> a*min + b and a*max + b, swapped when a < 0
        //               (multiplying by a negative number reverses the order).
        Residue new_sum = mod_p((int128)a * blk.sum + (int128)b * len);
        Value new_mn = blk.mn, new_mx = blk.mx;
        if (exact()) {
            new_mn = apply(a, b, blk.mn);
            new_mx = apply(a, b, blk.mx);
            if (a < 0) std::swap(new_mn, new_mx);
        }

        // New pending tag: the old tag acts first, then the new one.
        //   a*(old_a*x + old_b) + b = (a*old_a)*x + (a*old_b + b)
        Value new_a = apply(a, 0, blk.a);
        Value new_b = apply(a, b, blk.b);

        // Everything is computed before writing, so an overflow above leaves the block unchanged.
        blk.sum = new_sum;
        blk.mn = new_mn;
        blk.mx = new_mx;
        blk.a = new_a;
        blk.b = new_b;
    }

    // Only [lo, hi) inside block k is updated: O(block size).
    // One tag cannot mean "update only some elements", so we work on the raw values.
    void partial_update(std::size_t k, std::size_t lo, std::size_t hi, Value a, Value b) {
        push_tag(k);
        for (std::size_t i = lo; i < hi; i++) raw[i] = apply(a, b, raw[i]);
        recompute_block(k);
        counters.partial_elements += block_end(k) - block_start(k);
    }

    void update(Range r, AffineTag tag) {
        check_built_and_range(r, "update");
        Value a = tag.a, b = tag.b;
        if (exact() && !allowed_exact_gain(a))
            throw std::invalid_argument("SqrtDecomposition::update: exact_extrema requires gain in {-1,0,1}");
        if (r.begin == r.end) return;
        if (!exact()) {  // any int64 is allowed; reduce it mod P first
            a = mod_p(a);
            b = mod_p(b);
        }

        std::size_t first = r.begin / block_size;
        std::size_t last = (r.end - 1) / block_size;
        for (std::size_t k = first; k <= last; k++) {
            std::size_t lo = std::max(r.begin, block_start(k));
            std::size_t hi = std::min(r.end, block_end(k));
            if (lo == block_start(k) && hi == block_end(k)) full_update(k, a, b);
            else partial_update(k, lo, hi, a, b);
        }
    }

    Aggregate query(Range r) {
        check_built_and_range(r, "query");
        Aggregate result;
        if (r.begin == r.end) return result;  // length 0, sum 0, no min/max

        Residue sum = 0;
        Value mn = INT64_MAX, mx = INT64_MIN;
        std::size_t first = r.begin / block_size;
        std::size_t last = (r.end - 1) / block_size;
        for (std::size_t k = first; k <= last; k++) {
            std::size_t lo = std::max(r.begin, block_start(k));
            std::size_t hi = std::min(r.end, block_end(k));
            const Block& blk = blocks[k];

            if (lo == block_start(k) && hi == block_end(k)) {
                // Whole block: the summary already includes the tag. O(1).
                counters.full_blocks++;
                sum = (sum + blk.sum) % modulus;
                mn = std::min(mn, blk.mn);
                mx = std::max(mx, blk.mx);
            } else {
                // Part of a block: compute each real value a*raw + b. O(hi - lo).
                counters.partial_elements += hi - lo;
                int128 part_sum = 0;
                for (std::size_t i = lo; i < hi; i++) {
                    Value v = apply(blk.a, blk.b, raw[i]);
                    part_sum += v;
                    mn = std::min(mn, v);
                    mx = std::max(mx, v);
                }
                sum = mod_p(sum + part_sum);
            }
        }
        counters.aggregate_merges += last - first;  // block results combined after the first one

        result.length = r.end - r.begin;
        result.sum_mod = sum;
        if (exact()) {
            result.minimum = mn;
            result.maximum = mx;
        }
        return result;
    }

    // Recompute every block from its raw values and tag, and compare with what is stored.
    // Only reads: it never fixes anything and never changes the counters.
    InvariantReport check() const {
        InvariantReport report;
        report.ok = true;
        if (!built) {
            report_problem(report, "not built");
            return report;
        }
        if (raw.size() != n) report_problem(report, "raw size != n");
        if (blocks.size() != (n + block_size - 1) / block_size) {
            report_problem(report, "block count mismatch");
            return report;
        }

        for (std::size_t k = 0; k < blocks.size(); k++) {
            const Block& blk = blocks[k];
            std::string where = " in block " + std::to_string(k);
            if (blk.sum < 0 || blk.sum >= modulus) report_problem(report, "sum not canonical" + where);
            if (exact() && !allowed_exact_gain(blk.a)) report_problem(report, "pending gain outside {-1,0,1}" + where);
            if (!exact() && (blk.a < 0 || blk.a >= modulus || blk.b < 0 || blk.b >= modulus))
                report_problem(report, "pending tag not canonical" + where);

            int128 sum = 0, mn = INT64_MAX, mx = INT64_MIN;
            bool overflow = false;
            for (std::size_t i = block_start(k); i < block_end(k); i++) {
                if (!exact() && (raw[i] < 0 || raw[i] >= modulus)) report_problem(report, "raw residue not canonical" + where);
                int128 real = (int128)blk.a * raw[i] + blk.b;
                if (exact() && (real < INT64_MIN || real > INT64_MAX)) {
                    overflow = true;
                    break;
                }
                sum += real;
                if (real < mn) mn = real;
                if (real > mx) mx = real;
            }
            if (overflow) {
                report_problem(report, "real value overflows int64" + where);
                continue;
            }
            if (mod_p(sum) != blk.sum) report_problem(report, "block sum != recomputed sum" + where);
            if (exact() && (blk.mn != mn || blk.mx != mx)) report_problem(report, "block min/max != recomputed min/max" + where);
        }
        return report;
    }
};

SqrtDecomposition::SqrtDecomposition() : impl_(std::make_unique<Impl>()) {}
SqrtDecomposition::~SqrtDecomposition() = default;

void SqrtDecomposition::build(const std::vector<Value>& initial, Mode mode) { impl_->build(initial, mode); }
std::size_t SqrtDecomposition::size() const noexcept { return impl_->n; }
Mode SqrtDecomposition::mode() const noexcept { return impl_->mode; }
void SqrtDecomposition::update(Range range, AffineTag tag) { impl_->update(range, tag); }
Aggregate SqrtDecomposition::query(Range range) { return impl_->query(range); }
Counters SqrtDecomposition::counters() const noexcept { return impl_->counters; }
void SqrtDecomposition::reset_counters() noexcept { impl_->counters = {}; }
InvariantReport SqrtDecomposition::check_invariants() const { return impl_->check(); }

}  // namespace cs253
