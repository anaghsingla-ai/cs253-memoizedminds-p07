// Algorithm tests for Structure B (SqrtDecomposition).
// ArrayOracle is not implemented yet (placeholder), so this file carries its own minimal
// independent plain-array reference (RefArray). It shares no arithmetic with the structure.
// Once the Oracle owner lands ArrayOracle, add it as a second comparator; do not delete this.
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/sqrt_config.hpp"
#include <cstdint>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <vector>

using namespace cs253;

namespace {

int failures = 0;
#define CHECK(cond)                                                                     \
    do {                                                                                \
        if (!(cond)) {                                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                 \
            ++failures;                                                                 \
        }                                                                               \
    } while (0)

struct RefArray {
    Mode mode;
    std::vector<__int128_t> v;  // exact: true values; modular: residues
    static std::int64_t red(__int128_t x) {
        std::int64_t r = static_cast<std::int64_t>(x % 998244353);
        return r < 0 ? r + 998244353 : r;
    }
    RefArray(const std::vector<Value>& init, Mode m) : mode(m) {
        for (Value x : init) v.push_back(m == Mode::exact_extrema ? (__int128_t)x : (__int128_t)red(x));
    }
    void update(Range r, AffineTag t) {
        __int128_t a = t.a, b = t.b;
        if (mode == Mode::modular_sum) { a = red(a); b = red(b); }
        for (std::size_t i = r.begin; i < r.end; ++i) {
            v[i] = a * v[i] + b;
            if (mode == Mode::modular_sum) v[i] = red(v[i]);
        }
    }
    Aggregate query(Range r) const {
        Aggregate g;
        g.length = r.end - r.begin;
        if (g.length == 0) return g;
        __int128_t s = 0, mn = v[r.begin], mx = v[r.begin];
        for (std::size_t i = r.begin; i < r.end; ++i) {
            s += v[i];
            if (v[i] < mn) mn = v[i];
            if (v[i] > mx) mx = v[i];
        }
        g.sum_mod = red(s);
        if (mode == Mode::exact_extrema) { g.minimum = (Value)mn; g.maximum = (Value)mx; }
        return g;
    }
};

bool same(const Aggregate& x, const Aggregate& y) {
    return x.length == y.length && x.sum_mod == y.sum_mod && x.minimum == y.minimum &&
           x.maximum == y.maximum;
}

std::vector<Value> make_initial(std::mt19937_64& g, std::size_t n, Mode m) {
    std::vector<Value> r(n);
    for (auto& x : r) {
        if (m == Mode::exact_extrema) x = (Value)(g() % 2001) - 1000;
        else x = (Value)g();  // full int64 range, including negatives
    }
    return r;
}

AffineTag make_tag(std::mt19937_64& g, Mode m) {
    if (m == Mode::exact_extrema) {
        static const Value gains[] = {-1, 0, 1};
        return {gains[g() % 3], (Value)(g() % 2001) - 1000};
    }
    switch (g() % 6) {
        case 0: return {INT64_MIN, INT64_MIN};
        case 1: return {INT64_MAX, INT64_MIN};
        case 2: return {-1, (Value)g()};
        case 3: return {998244353, 998244353LL * 3};  // reduces to (0,0)
        default: return {(Value)g(), (Value)g()};
    }
}

Range make_range(std::mt19937_64& g, std::size_t n, std::size_t b) {
    std::size_t lo, hi;
    if (g() % 3 == 0 && b > 0) {  // bias towards block boundaries +-1
        auto pick = [&]() {
            std::size_t k = g() % (n / b + 2);
            std::size_t p = k * b;
            int d = (int)(g() % 3) - 1;
            long long q = (long long)p + d;
            if (q < 0) q = 0;
            if (q > (long long)n) q = (long long)n;
            return (std::size_t)q;
        };
        lo = pick(); hi = pick();
    } else {
        lo = g() % (n + 1); hi = g() % (n + 1);
    }
    if (lo > hi) std::swap(lo, hi);
    return {lo, hi};
}

// Differential run. sz = block-size override (0 = automatic).
void differential(std::uint64_t seed, std::size_t n, Mode m, std::size_t block_override, int ops) {
    set_sqrt_block_size(block_override);
    std::mt19937_64 g(seed);
    auto init = make_initial(g, n, m);
    SqrtDecomposition s;
    s.build(init, m);
    RefArray ref(init, m);
    std::size_t b = block_override;
    if (b == 0) { b = 1; while (b * b < n) ++b; }
    CHECK(s.check_invariants().ok);
    for (int i = 0; i < ops; ++i) {
        Range r = make_range(g, n, b);
        if (g() % 2) {
            AffineTag t = make_tag(g, m);
            s.update(r, t);
            ref.update(r, t);
        } else {
            Aggregate got = s.query(r), want = ref.query(r);
            if (!same(got, want)) {
                std::printf("MISMATCH seed=%llu n=%zu mode=%d b=%zu op=%d [%zu,%zu)\n",
                            (unsigned long long)seed, n, (int)m, b, i, r.begin, r.end);
                ++failures;
                return;
            }
        }
        auto rep = s.check_invariants();
        if (!rep.ok) {
            std::printf("INVARIANT seed=%llu n=%zu op=%d: %s\n", (unsigned long long)seed, n, i,
                        rep.violations.empty() ? "?" : rep.violations[0].c_str());
            ++failures;
            return;
        }
    }
    // Whole-array and every single element at the end.
    CHECK(same(s.query({0, n}), ref.query({0, n})));
    for (std::size_t i = 0; i < n; ++i) CHECK(same(s.query({i, i + 1}), ref.query({i, i + 1})));
    set_sqrt_block_size(0);
}

void test_contract_examples() {
    set_sqrt_block_size(0);
    // +3 then negate => -x-3 ; negate then +3 => -x+3 (single element, all mapped identically)
    for (std::size_t n : {1u, 5u, 9u}) {
        std::vector<Value> init(n, 10);
        SqrtDecomposition a, c;
        a.build(init, Mode::exact_extrema);
        c.build(init, Mode::exact_extrema);
        a.update({0, n}, {1, 3});
        a.update({0, n}, {-1, 0});
        c.update({0, n}, {-1, 0});
        c.update({0, n}, {1, 3});
        CHECK(*a.query({0, n}).minimum == -13 && *a.query({0, n}).maximum == -13);
        CHECK(*c.query({0, n}).minimum == -7 && *c.query({0, n}).maximum == -7);
    }
    // reset then shift, and negative gain swapping extrema
    SqrtDecomposition s;
    s.build({5, -2, 9, 0, 7, 3, 1}, Mode::exact_extrema);
    s.update({1, 6}, {-1, 0});  // partial blocks, block size 3
    Aggregate q = s.query({0, 7});
    CHECK(*q.minimum == -9 && *q.maximum == 5);
    s.update({0, 7}, {0, 4});  // reset
    q = s.query({0, 7});
    CHECK(*q.minimum == 4 && *q.maximum == 4 && q.sum_mod == 28);
    s.update({2, 5}, {1, -10});
    q = s.query({0, 7});
    CHECK(*q.minimum == -6 && *q.maximum == 4 && q.sum_mod == (28 - 30 + 998244353) % 998244353);
}

void test_errors_and_edges() {
    set_sqrt_block_size(0);
    SqrtDecomposition s;
    bool threw = false;
    try { s.update({0, 0}, {1, 0}); } catch (const std::logic_error&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { s.query({0, 0}); } catch (const std::logic_error&) { threw = true; }
    CHECK(threw);

    s.build({}, Mode::exact_extrema);  // empty build
    CHECK(s.size() == 0);
    Aggregate e = s.query({0, 0});
    CHECK(e.length == 0 && e.sum_mod == 0 && !e.minimum && !e.maximum);
    s.update({0, 0}, {1, 1});
    CHECK(s.check_invariants().ok);

    s.build({1, 2, 3, 4, 5}, Mode::exact_extrema);
    threw = false;
    try { s.update({0, 6}, {1, 1}); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { s.update({3, 2}, {1, 1}); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { s.query({0, 6}); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { s.update({0, 5}, {2, 0}); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    // nothing mutated by the failed calls
    Aggregate q = s.query({0, 5});
    CHECK(*q.minimum == 1 && *q.maximum == 5 && q.sum_mod == 15);
    Aggregate z = s.query({2, 2});
    CHECK(z.length == 0 && !z.minimum);

    // modular mode: any gain accepted, extrema absent, build resets counters
    s.build({1, 2, 3}, Mode::modular_sum);
    s.update({0, 3}, {INT64_MIN, INT64_MAX});
    q = s.query({0, 3});
    CHECK(!q.minimum && !q.maximum);
    CHECK(q.sum_mod >= 0 && q.sum_mod < 998244353);
    s.build({1}, Mode::exact_extrema);
    CHECK(s.counters().full_blocks == 0 && s.counters().partial_elements == 0);
    s.update({0, 1}, {-1, 0});
    CHECK(s.counters().full_blocks == 1);
    s.reset_counters();
    CHECK(s.counters().full_blocks == 0 && s.counters().tag_compositions == 0);
    // exact overflow is reported, not wrapped
    s.build({INT64_MIN}, Mode::exact_extrema);
    threw = false;
    try { s.update({0, 1}, {-1, 0}); } catch (const std::overflow_error&) { threw = true; }
    CHECK(threw);
}

// Counter units as documented in DESIGN.md, on n = 9 with automatic block size 3.
void test_counters() {
    set_sqrt_block_size(0);
    SqrtDecomposition s;
    s.build({1, 2, 3, 4, 5, 6, 7, 8, 9}, Mode::exact_extrema);
    s.update({1, 8}, {1, 5});  // block 0 partial, block 1 full, block 2 partial
    Counters c = s.counters();
    CHECK(c.full_blocks == 1 && c.partial_elements == 6 && c.tag_compositions == 1);
    CHECK(c.aggregate_merges == 0 && c.nodes_visited == 0 && c.push_downs == 0);
    s.reset_counters();
    s.query({0, 9});  // three full blocks, two merges
    c = s.counters();
    CHECK(c.full_blocks == 3 && c.partial_elements == 0 && c.aggregate_merges == 2);
    s.reset_counters();
    s.query({1, 5});  // two elements of block 0, two elements of block 1
    c = s.counters();
    CHECK(c.full_blocks == 0 && c.partial_elements == 4 && c.aggregate_merges == 1);
    CHECK(c.tag_compositions == 0);
    CHECK(s.check_invariants().ok);
}

// A full-block update that overflows must throw before changing that block.
void test_overflow_leaves_block_unchanged() {
    set_sqrt_block_size(0);
    SqrtDecomposition s;
    s.build({INT64_MAX - 5, 7, 8, 9}, Mode::exact_extrema);  // block size 2
    bool threw = false;
    try { s.update({0, 2}, {1, 10}); } catch (const std::overflow_error&) { threw = true; }
    CHECK(threw);
    Aggregate q = s.query({0, 2});
    CHECK(*q.maximum == INT64_MAX - 5 && *q.minimum == 7);
    CHECK(s.check_invariants().ok);

    // The other block is unaffected and the structure stays usable.
    s.update({2, 4}, {-1, 0});
    q = s.query({2, 4});
    CHECK(*q.minimum == -9 && *q.maximum == -8);
}

// Large magnitudes (near +-2^61) exercise the 128-bit sum and extrema paths in exact mode.
void test_large_values() {
    for (std::size_t b : {0u, 1u, 5u}) {
        set_sqrt_block_size(b);
        std::mt19937_64 g(99 + b);
        const std::size_t n = 200;
        std::vector<Value> init(n);
        for (auto& x : init) x = (Value)(g() % (1ULL << 62)) - (Value)(1LL << 61);
        SqrtDecomposition s;
        s.build(init, Mode::exact_extrema);
        RefArray ref(init, Mode::exact_extrema);
        std::size_t bs = b == 0 ? 15 : b;
        for (int i = 0; i < 2000; ++i) {
            Range r = make_range(g, n, bs);
            if (g() % 2) {
                static const Value gains[] = {-1, 0, 1};
                AffineTag t{gains[g() % 3], (Value)(g() % 2001) - 1000};
                s.update(r, t);
                ref.update(r, t);
            } else {
                CHECK(same(s.query(r), ref.query(r)));
            }
        }
        CHECK(s.check_invariants().ok);
        CHECK(same(s.query({0, n}), ref.query({0, n})));
    }
    set_sqrt_block_size(0);
}

}  // namespace

int main() {
    test_contract_examples();
    test_errors_and_edges();
    test_counters();
    test_overflow_leaves_block_unchanged();
    test_large_values();

    int runs = 0;
    for (Mode m : {Mode::exact_extrema, Mode::modular_sum}) {
        // every size 1..70 (covers perfect squares, primes, short last block), auto block size
        for (std::size_t n = 1; n <= 70; ++n) {
            differential(1000 + n, n, m, 0, 300);
            ++runs;
        }
        // explicit block sizes incl. 1 (all full blocks), n (single block) and non-divisors
        for (std::size_t b : {1u, 2u, 3u, 7u, 16u, 50u, 1000u})
            for (std::size_t n : {1u, 2u, 15u, 16u, 17u, 49u, 50u, 51u, 100u, 257u}) {
                differential(77 * b + n, n, m, b, 400);
                ++runs;
            }
        differential(424242, 5000, m, 0, 3000);  // larger, non-power-of-two
        ++runs;
    }
    std::printf("structB_test: %d differential runs, %d failures\n", runs, failures);
    return failures == 0 ? 0 : 1;
}
