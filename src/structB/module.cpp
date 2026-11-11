// Structure B: square-root decomposition with per-block affine lazy tags.
// Design, invariants and complexity: src/structB/DESIGN.md. Proofs: docs/derivations/D2.md.
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/sqrt_config.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace cs253 {

namespace {

using Wide = __int128_t;

std::size_t g_block_setting = 0;

Residue norm(Wide x) {
    x %= modulus;
    if (x < 0) x += modulus;
    return static_cast<Residue>(x);
}

Value checked(Wide x) {
    if (x < std::numeric_limits<Value>::min() || x > std::numeric_limits<Value>::max())
        throw std::overflow_error("exact affine arithmetic overflow");
    return static_cast<Value>(x);
}

bool is_identity(const AffineTag& t) { return t.a == 1 && t.b == 0; }

bool exact_gain_ok(Value a) { return a == -1 || a == 0 || a == 1; }

}  // namespace

void set_sqrt_block_size(std::size_t block_size) noexcept { g_block_setting = block_size; }
std::size_t sqrt_block_size_setting() noexcept { return g_block_setting; }

struct SqrtDecomposition::Impl {
    // Logical value of element i = tag(raw[i]) where tag is its block's pending tag.
    // Block aggregates (sum, mn, mx) are LOGICAL: they already include the pending tag.
    struct Block {
        Value a = 1, b = 0;       // pending tag x -> a*x+b (modular mode: residues)
        Residue sum = 0;          // logical sum mod P
        Value mn = 0, mx = 0;     // logical extrema (exact mode only)
    };

    bool built = false;
    Mode md = Mode::exact_extrema;
    std::size_t n = 0, bsize = 1, nblocks = 0;
    std::vector<Value> raw;       // exact: raw values; modular: canonical residues
    std::vector<Block> blocks;
    Counters cnt;

    bool exact() const { return md == Mode::exact_extrema; }
    std::size_t bstart(std::size_t k) const { return k * bsize; }
    std::size_t bend(std::size_t k) const { return std::min(n, (k + 1) * bsize); }

    // newer AFTER older: (a2*a1, a2*b1+b2)
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

    // Transform extrema of a set by gain a in {-1,0,1} then offset b.
    static void map_extrema(Value a, Value b, Value mn, Value mx, Value& omn, Value& omx) {
        if (a == 1) {
            omn = checked(Wide(mn) + b);
            omx = checked(Wide(mx) + b);
        } else if (a == -1) {  // order reverses: old max becomes new min
            omn = checked(-Wide(mx) + b);
            omx = checked(-Wide(mn) + b);
        } else {
            omn = omx = b;
        }
    }

    // Tag t applied to a set of `len` elements with raw sum S (Wide) and raw extrema.
    void map_aggregate(AffineTag t, Wide raw_sum, std::size_t len, Value rmn, Value rmx,
                       Residue& sum, Value& mn, Value& mx) const {
        sum = norm(Wide(t.a) * raw_sum + Wide(t.b) * Wide(len));
        if (exact()) map_extrema(t.a, t.b, rmn, rmx, mn, mx);
    }

    // Recompute a block's logical aggregate from raw[] and its pending tag, read-only.
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
    }

    void build(const std::vector<Value>& initial, Mode mode) {
        std::size_t nn = initial.size();
        std::size_t bs = g_block_setting;
        if (bs == 0) {
            bs = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(nn))));
            while (bs * bs < nn) ++bs;  // guard against FP rounding
        }
        if (bs < 1) bs = 1;
        std::vector<Value> r;
        r.reserve(nn);
        for (Value v : initial) r.push_back(mode == Mode::exact_extrema ? v : norm(v));
        std::size_t nb = nn == 0 ? 0 : (nn + bs - 1) / bs;
        std::vector<Block> bl(nb);

        raw = std::move(r);
        blocks = std::move(bl);
        n = nn; bsize = bs; nblocks = nb; md = mode;
        for (std::size_t k = 0; k < nblocks; ++k) {
            Wide s; Value mn = 0, mx = 0;
            scan(bstart(k), bend(k), s, mn, mx);
            blocks[k].sum = norm(s);
            if (exact()) { blocks[k].mn = mn; blocks[k].mx = mx; }
        }
        cnt = {};
        built = true;
    }

    void require_built() const {
        if (!built) throw std::logic_error("SqrtDecomposition: build() has not been called");
    }
    void check_range(Range r, const char* what) const {
        if (r.begin > r.end || r.end > n) throw std::out_of_range(what);
    }

    // Full-block update: O(1). Aggregates updated by formula, tag composed (newer after older).
    void full_update(std::size_t k, AffineTag t) {
        ++cnt.full_blocks;
        Block& B = blocks[k];
        std::size_t len = bend(k) - bstart(k);
        Residue new_sum = norm(Wide(norm(t.a)) * B.sum + Wide(norm(t.b)) * Wide(len));
        Value nmn = B.mn, nmx = B.mx;
        if (exact()) map_extrema(t.a, t.b, B.mn, B.mx, nmn, nmx);
        AffineTag nt = compose(t, {B.a, B.b});  // may throw before any mutation of B
        B.sum = new_sum; B.mn = nmn; B.mx = nmx; B.a = nt.a; B.b = nt.b;
    }

    // Partial update: one fused pass = materialize pending tag + apply new tag on [lo,hi) + rebuild.
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
        cnt.partial_elements += be - bs;
        B.a = 1; B.b = 0;
        B.sum = norm(s);
        if (exact()) { B.mn = mn; B.mx = mx; }
    }

    // Partial query: read-only; applies the pending tag to the raw aggregate of the overlap.
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

    void update(Range r, AffineTag t) {
        require_built();
        check_range(r, "SqrtDecomposition::update: invalid range");
        if (exact() && !exact_gain_ok(t.a))
            throw std::invalid_argument("SqrtDecomposition::update: exact_extrema requires gain in {-1,0,1}");
        if (r.begin == r.end) return;
        if (!exact()) { t.a = norm(t.a); t.b = norm(t.b); }
        std::size_t kl = r.begin / bsize, kr = (r.end - 1) / bsize;
        for (std::size_t k = kl; k <= kr; ++k) {
            std::size_t lo = std::max(r.begin, bstart(k)), hi = std::min(r.end, bend(k));
            if (lo == bstart(k) && hi == bend(k)) full_update(k, t);
            else partial_update(k, lo, hi, t);
        }
    }

    Aggregate query(Range r) {
        require_built();
        check_range(r, "SqrtDecomposition::query: invalid range");
        if (r.begin == r.end) return {};
        Aggregate acc;
        bool first = true;
        auto fold = [&](const Aggregate& part) {
            if (first) { acc = part; first = false; return; }
            ++cnt.aggregate_merges;
            acc.length += part.length;
            acc.sum_mod = norm(Wide(acc.sum_mod) + part.sum_mod);
            if (exact()) {
                acc.minimum = std::min(*acc.minimum, *part.minimum);
                acc.maximum = std::max(*acc.maximum, *part.maximum);
            }
        };
        std::size_t kl = r.begin / bsize, kr = (r.end - 1) / bsize;
        for (std::size_t k = kl; k <= kr; ++k) {
            std::size_t lo = std::max(r.begin, bstart(k)), hi = std::min(r.end, bend(k));
            if (lo == bstart(k) && hi == bend(k)) {
                ++cnt.full_blocks;
                const Block& B = blocks[k];
                Aggregate part;
                part.length = hi - lo;
                part.sum_mod = B.sum;
                if (exact()) { part.minimum = B.mn; part.maximum = B.mx; }
                fold(part);
            } else {
                fold(partial_query(k, lo, hi));
            }
        }
        return acc;
    }

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
                Wide y = Wide(B.a) * x + B.b;  // logical value
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
        return rep;
    }
};

SqrtDecomposition::SqrtDecomposition() : impl_(std::make_unique<Impl>()) {}
SqrtDecomposition::~SqrtDecomposition() = default;

void SqrtDecomposition::build(const std::vector<Value>& initial, Mode mode) { impl_->build(initial, mode); }
std::size_t SqrtDecomposition::size() const noexcept { return impl_->n; }
Mode SqrtDecomposition::mode() const noexcept { return impl_->md; }
void SqrtDecomposition::update(Range range, AffineTag tag) { impl_->update(range, tag); }
Aggregate SqrtDecomposition::query(Range range) { return impl_->query(range); }
Counters SqrtDecomposition::counters() const noexcept { return impl_->cnt; }
void SqrtDecomposition::reset_counters() noexcept { impl_->cnt = {}; }
InvariantReport SqrtDecomposition::check_invariants() const { return impl_->check(); }

}  // namespace cs253
