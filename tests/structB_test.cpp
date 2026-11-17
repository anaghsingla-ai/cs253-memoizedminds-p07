#include "cs253/sqrt_config.hpp"
#include "cs253/sqrt_decomposition.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <vector>

using namespace cs253;

namespace {

int failures = 0;

// CHECK records a failed test and lets the remaining tests continue.
#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            ++failures; \
        } \
    } while (0)

// A simple array used to check the optimized data structure.
// It updates and queries elements one by one.
struct SimpleArray {
    Mode mode;
    std::vector<__int128_t> v;
    static std::int64_t modValue(__int128_t x) {
        std::int64_t r = static_cast<std::int64_t>(x % 998244353);
        return r < 0 ? r + 998244353 : r;
    }
    SimpleArray(const std::vector<Value>& init, Mode m) : mode(m) {
        for (Value x : init) v.push_back(m == Mode::exact_extrema ? (__int128_t)x : (__int128_t)modValue(x));
    }
    void update(Range r, AffineTag t) {
        __int128_t a = t.a, b = t.b;
        if (mode == Mode::modular_sum) { a = modValue(a); b = modValue(b); }
        for (std::size_t i = r.begin; i < r.end; ++i) {
            v[i] = a * v[i] + b;
            if (mode == Mode::modular_sum) v[i] = modValue(v[i]);
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
        g.sum_mod = modValue(s);
        if (mode == Mode::exact_extrema) { g.minimum = (Value)mn; g.maximum = (Value)mx; }
        return g;
    }
};

bool sameResult(const Aggregate& x, const Aggregate& y) {
    return x.length == y.length && x.sum_mod == y.sum_mod && x.minimum == y.minimum &&
           x.maximum == y.maximum;
}

std::vector<Value> makeArray(std::mt19937_64& g, std::size_t n, Mode m) {
    std::vector<Value> r(n);
    for (auto& x : r) {
        if (m == Mode::exact_extrema) x = (Value)(g() % 2001) - 1000;
        else x = (Value)g();
    }
    return r;
}

AffineTag makeUpdate(std::mt19937_64& g, Mode m) {
    if (m == Mode::exact_extrema) {
        static const Value gains[] = {-1, 0, 1};
        return {gains[g() % 3], (Value)(g() % 2001) - 1000};
    }
    switch (g() % 6) {
        case 0: return {INT64_MIN, INT64_MIN};
        case 1: return {INT64_MAX, INT64_MIN};
        case 2: return {-1, (Value)g()};
        case 3: return {998244353, 998244353LL * 3};
        default: return {(Value)g(), (Value)g()};
    }
}

Range makeRange(std::mt19937_64& g, std::size_t n, std::size_t b) {
    std::size_t lo, hi;
    if (g() % 3 == 0 && b > 0) {
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


// Run the same random operations on both implementations and compare results.
void testRandomOperations(std::uint64_t seed, std::size_t n, Mode m, std::size_t block_override, int ops) {
    set_sqrt_block_size(block_override);
    std::mt19937_64 g(seed);
    auto init = makeArray(g, n, m);
    SqrtDecomposition s;
    s.build(init, m);
    SimpleArray ref(init, m);
    std::size_t b = block_override;
    if (b == 0) { b = 1; while (b * b < n) ++b; }
    CHECK(s.check_invariants().ok);
    for (int i = 0; i < ops; ++i) {
        Range r = makeRange(g, n, b);
        if (g() % 2) {
            AffineTag t = makeUpdate(g, m);
            s.update(r, t);
            ref.update(r, t);
        } else {
            Aggregate got = s.query(r), want = ref.query(r);
            if (!sameResult(got, want)) {
                std::printf("MISMATCH seed=%llu n=%zu mode=%d b=%zu op=%d [%zu,%zu)\n",
                            (unsigned long long)seed, n, (int)m, b, i, r.begin, r.end);
                ++failures;
                return;
            }
        }
        // After every operation the structure must still be internally consistent.
        auto rep = s.check_invariants();
        if (!rep.ok) {
            std::printf("INVARIANT seed=%llu n=%zu op=%d: %s\n", (unsigned long long)seed, n, i,
                        rep.violations.empty() ? "?" : rep.violations[0].c_str());
            ++failures;
            return;
        }
    }

    // Finally compare the whole array and every single element.
    CHECK(sameResult(s.query({0, n}), ref.query({0, n})));
    for (std::size_t i = 0; i < n; ++i) CHECK(sameResult(s.query({i, i + 1}), ref.query({i, i + 1})));
    set_sqrt_block_size(0);
}

// Hand-checked examples from INTERFACE.md.
void testExamples() {
    set_sqrt_block_size(0);

    // "+3 then negate" gives -x-3, but "negate then +3" gives -x+3: order matters.
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

    // Negative gain swaps min and max; gain 0 resets every value.
    SqrtDecomposition s;
    s.build({5, -2, 9, 0, 7, 3, 1}, Mode::exact_extrema);
    s.update({1, 6}, {-1, 0});
    Aggregate q = s.query({0, 7});
    CHECK(*q.minimum == -9 && *q.maximum == 5);
    s.update({0, 7}, {0, 4});
    q = s.query({0, 7});
    CHECK(*q.minimum == 4 && *q.maximum == 4 && q.sum_mod == 28);
    s.update({2, 5}, {1, -10});
    q = s.query({0, 7});
    CHECK(*q.minimum == -6 && *q.maximum == 4 && q.sum_mod == (28 - 30 + 998244353) % 998244353);
}

// Errors and edge cases: use before build, empty arrays, invalid ranges and gains.
void testErrors() {
    set_sqrt_block_size(0);
    SqrtDecomposition s;
    bool threw = false;
    try { s.update({0, 0}, {1, 0}); } catch (const std::logic_error&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { s.query({0, 0}); } catch (const std::logic_error&) { threw = true; }
    CHECK(threw);

    s.build({}, Mode::exact_extrema);
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

    // The failed calls above must not have changed anything.
    Aggregate q = s.query({0, 5});
    CHECK(*q.minimum == 1 && *q.maximum == 5 && q.sum_mod == 15);
    Aggregate z = s.query({2, 2});
    CHECK(z.length == 0 && !z.minimum);

    // Modular mode: any gain is allowed and min/max are absent.
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

    s.build({INT64_MIN}, Mode::exact_extrema);
    threw = false;
    try { s.update({0, 1}, {-1, 0}); } catch (const std::overflow_error&) { threw = true; }
    CHECK(threw);
}


// Check that the operation counters are updated correctly.
void testCounters() {
    set_sqrt_block_size(0);
    SqrtDecomposition s;
    s.build({1, 2, 3, 4, 5, 6}, Mode::exact_extrema);
    s.update({0, 6}, {1, 2});
    CHECK(s.counters().full_blocks == 2);
    CHECK(s.counters().tag_compositions == 2);

    s.reset_counters();
    (void)s.query({1, 5});
    CHECK(s.counters().partial_elements == 4);
    CHECK(s.counters().aggregate_merges == 1);
    CHECK(s.counters().full_blocks == 0);
    CHECK(s.counters().tag_compositions == 0);
}

// Check that a failed overflowing update does not change this block.
void testOverflow() {
    set_sqrt_block_size(2);
    SqrtDecomposition s;
    s.build({INT64_MAX, 0}, Mode::exact_extrema);
    const Aggregate before = s.query({0, 2});
    bool threw = false;
    try {
        s.update({0, 2}, {1, 1});
    } catch (const std::overflow_error&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(sameResult(s.query({0, 2}), before));
    CHECK(s.check_invariants().ok);
    set_sqrt_block_size(0);
}

// Check calculations involving very large integer values.
void testLargeValues() {
    set_sqrt_block_size(0);
    const std::vector<Value> exact_values{INT64_MAX, INT64_MIN, 17, -29};
    SqrtDecomposition exact;
    exact.build(exact_values, Mode::exact_extrema);
    SimpleArray exact_ref(exact_values, Mode::exact_extrema);
    CHECK(sameResult(exact.query({0, exact_values.size()}),
               exact_ref.query({0, exact_values.size()})));
    exact.update({1, 4}, {0, INT64_MAX});
    exact_ref.update({1, 4}, {0, INT64_MAX});
    CHECK(sameResult(exact.query({0, exact_values.size()}),
               exact_ref.query({0, exact_values.size()})));
    CHECK(exact.check_invariants().ok);

    const std::vector<Value> modular_values{INT64_MIN, INT64_MAX, -1, 0};
    SqrtDecomposition modular;
    modular.build(modular_values, Mode::modular_sum);
    SimpleArray modular_ref(modular_values, Mode::modular_sum);
    modular.update({0, 4}, {INT64_MIN, INT64_MAX});
    modular_ref.update({0, 4}, {INT64_MIN, INT64_MAX});
    CHECK(sameResult(modular.query({0, 4}), modular_ref.query({0, 4})));
    CHECK(modular.check_invariants().ok);
}

}

int main() {
    testExamples();
    testErrors();
    testCounters();
    testOverflow();
    testLargeValues();

    int runs = 0;
    for (Mode m : {Mode::exact_extrema, Mode::modular_sum}) {

        for (std::size_t n = 1; n <= 70; ++n) {
            testRandomOperations(1000 + n, n, m, 0, 300);
            ++runs;
        }

        for (std::size_t b : {1u, 2u, 3u, 7u, 16u, 50u, 1000u})
            for (std::size_t n : {1u, 2u, 15u, 16u, 17u, 49u, 50u, 51u, 100u, 257u}) {
                testRandomOperations(77 * b + n, n, m, b, 400);
                ++runs;
            }
        testRandomOperations(424242, 5000, m, 0, 3000);
        ++runs;
    }
    std::printf("structB_test: %d random test runs, %d failures\n", runs, failures);
    return failures == 0 ? 0 : 1;
}
