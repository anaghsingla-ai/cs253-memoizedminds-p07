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