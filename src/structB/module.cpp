

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

}

void set_sqrt_block_size(std::size_t block_size) noexcept { chosen_block_size = block_size; }
std::size_t sqrt_block_size_setting() noexcept { return chosen_block_size; }

struct SqrtDecomposition::Impl {


    struct Block {
        Value a = 1, b = 0;
        Residue sum = 0;
        Value mn = 0, mx = 0;
    };

    bool built = false;
    Mode md = Mode::exact_extrema;
    std::size_t n = 0, bsize = 1, nblocks = 0;
    std::vector<Value> raw;
    std::vector<Block> blocks;
    Counters counters;

    bool exact() const { return mode == Mode::exact_extrema; }
    std::size_t block_start(std::size_t k) const { return k * block_size; }
    std::size_t block_end(std::size_t k) const { return std::min(n, (k + 1) * block_size); }  // last block may be shorter


    AffineTag compose(AffineTag newer, AffineTag older) {
        ++cnt.tag_compositions;
        if (exact())
            return {checked(Wide(newer.a) * older.a), checked(Wide(newer.a) * older.b + newer.b)};
        return {norm(Wide(newer.a) * older.a), norm(Wide(newer.a) * older.b + newer.b)};
    }

    Value apply_value(AffineTag t, Value x) const {
        if (exact()) return checked(Wide(t.a) * x + t.b);
        return norm(Wide(t.a) * x + t.b);
    }


    static void map_extrema(Value a, Value b, Value mn, Value mx, Value& omn, Value& omx) {
        if (a == 1) {
            omn = checked(Wide(mn) + b);
            omx = checked(Wide(mx) + b);
        } else if (a == -1) {
            omn = checked(-Wide(mx) + b);
            omx = checked(-Wide(mn) + b);
        } else {
            omn = omx = b;
        }
        blk.sum = mod_p(sum);
    }


    void map_aggregate(AffineTag t, Wide raw_sum, std::size_t len, Value rmn, Value rmx,
                       Residue& sum, Value& mn, Value& mx) const {
        sum = norm(Wide(t.a) * raw_sum + Wide(t.b) * Wide(len));
        if (exact()) map_extrema(t.a, t.b, rmn, rmx, mn, mx);
    }


    void scan(std::size_t lo, std::size_t hi, Wide& s, Value& mn, Value& mx) const {
        s = 0;
        if (exact()) {
            mn = std::numeric_limits<Value>::max();
            mx = std::numeric_limits<Value>::min();
            for (std::size_t i = lo; i < hi; ++i) {
                Value x = raw[i];
                s += x;
                if (x < mn) mn = x;
                if (x > mx) mx = x;
            }
        } else {
            for (std::size_t i = lo; i < hi; ++i) s += raw[i];
        }
        if (b == 0) b = 1;  // empty array

    void build(const std::vector<Value>& initial, Mode mode) {
        built = false;
        std::size_t nn = initial.size();
        std::size_t bs = g_block_setting;
        if (bs == 0) {
            bs = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(nn))));
            while (bs * bs < nn) ++bs;
        }
        if (bs < 1) bs = 1;
        std::vector<Value> r;
        r.reserve(nn);
        for (Value v : initial) r.push_back(mode == Mode::exact_extrema ? v : norm(v));
        std::size_t nb = nn == 0 ? 0 : (nn + bs - 1) / bs;
        std::vector<Block> bl(nb);

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


    void full_update(std::size_t k, AffineTag t) {
        ++cnt.full_blocks;
        Block& B = blocks[k];
        std::size_t len = bend(k) - bstart(k);
        Residue new_sum = norm(Wide(norm(t.a)) * B.sum + Wide(norm(t.b)) * Wide(len));
        Value nmn = B.mn, nmx = B.mx;
        if (exact()) map_extrema(t.a, t.b, B.mn, B.mx, nmn, nmx);
        AffineTag nt = compose(t, {B.a, B.b});
        B.sum = new_sum; B.mn = nmn; B.mx = nmx; B.a = nt.a; B.b = nt.b;
    }


    void partial_update(std::size_t k, std::size_t lo, std::size_t hi, AffineTag t) {
        Block& B = blocks[k];
        std::size_t bs = bstart(k), be = bend(k);
        AffineTag pend{B.a, B.b};
        bool has_pending = !is_identity(pend);
        Wide s = 0;
        Value mn = std::numeric_limits<Value>::max(), mx = std::numeric_limits<Value>::min();
        for (std::size_t i = bs; i < be; ++i) {
            Value x = raw[i];
            if (has_pending) x = apply_value(pend, x);
            if (i >= lo && i < hi) x = apply_value(t, x);
            raw[i] = x;
            s += x;
            if (x < mn) mn = x;
            if (x > mx) mx = x;
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


    Aggregate partial_query(std::size_t k, std::size_t lo, std::size_t hi) {
        const Block& B = blocks[k];
        Wide s; Value rmn = 0, rmx = 0;
        scan(lo, hi, s, rmn, rmx);
        cnt.partial_elements += hi - lo;
        Aggregate out;
        out.length = hi - lo;
        Value mn = 0, mx = 0;
        map_aggregate({B.a, B.b}, s, hi - lo, rmn, rmx, out.sum_mod, mn, mx);
        if (exact()) { out.minimum = mn; out.maximum = mx; }
        return out;
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
        InvariantReport rep{true, {}};
        auto bad = [&](std::string m) { rep.ok = false; rep.violations.push_back(std::move(m)); };
        if (!built) { bad("not built"); return rep; }
        if (raw.size() != n) bad("raw size != n");
        if (blocks.size() != (n == 0 ? 0 : (n + bsize - 1) / bsize)) { bad("block count mismatch"); return rep; }
        for (std::size_t k = 0; k < nblocks; ++k) {
            const Block& B = blocks[k];
            std::string tag = " in block " + std::to_string(k);
            if (B.sum < 0 || B.sum >= modulus) bad("sum not canonical" + tag);
            if (exact()) {
                if (!exact_gain_ok(B.a)) bad("pending gain outside {-1,0,1}" + tag);
            } else {
                if (B.a < 0 || B.a >= modulus || B.b < 0 || B.b >= modulus) bad("pending tag not canonical" + tag);
            }
            Wide s = 0;
            Value mn = std::numeric_limits<Value>::max(), mx = std::numeric_limits<Value>::min();
            bool overflow = false;
            for (std::size_t i = bstart(k); i < bend(k); ++i) {
                Value x = raw[i];
                if (!exact() && (x < 0 || x >= modulus)) bad("raw residue not canonical" + tag);
                Wide y = Wide(B.a) * x + B.b;
                if (exact()) {
                    if (y < std::numeric_limits<Value>::min() || y > std::numeric_limits<Value>::max()) { overflow = true; break; }
                    mn = std::min(mn, static_cast<Value>(y));
                    mx = std::max(mx, static_cast<Value>(y));
                }
                s += y;
            }
            if (overflow) { bad("logical value overflows int64" + tag); continue; }
            if (norm(s) != B.sum) bad("block sum != recomputed logical sum" + tag);
            if (exact() && (B.mn != mn || B.mx != mx)) bad("block extrema != recomputed logical extrema" + tag);
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
std::size_t SqrtDecomposition::size() const { impl_->require_built(); return impl_->n; }
Mode SqrtDecomposition::mode() const { impl_->require_built(); return impl_->md; }
void SqrtDecomposition::update(Range range, AffineTag tag) { impl_->update(range, tag); }
Aggregate SqrtDecomposition::query(Range range) { return impl_->query(range); }
Counters SqrtDecomposition::counters() const noexcept { return impl_->counters; }
void SqrtDecomposition::reset_counters() noexcept { impl_->counters = {}; }
InvariantReport SqrtDecomposition::check_invariants() const { return impl_->check(); }

}
