
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
    std::vector<__int128_t> v;

    static std::int64_t red(__int128_t x) {
        std::int64_t r = static_cast<std::int64_t>(x % 998244353);
        return r < 0 ? r + 998244353 : r;
    }

    RefArray(const std::vector<Value>& init, Mode m) : mode(m) {
        for (Value x : init) {
            v.push_back(
                m == Mode::exact_extrema
                    ? (__int128_t)x
                    : (__int128_t)red(x)
            );
        }
    }

    void update(Range r, AffineTag t) {
        __int128_t a = t.a, b = t.b;

        if (mode == Mode::modular_sum) {
            a = red(a);
            b = red(b);
        }

        for (std::size_t i = r.begin; i < r.end; ++i) {
            v[i] = a * v[i] + b;

            if (mode == Mode::modular_sum) {
                v[i] = red(v[i]);
            }
        }
    }

    Aggregate query(Range r) const {
        Aggregate g;
        g.length = r.end - r.begin;

        if (g.length == 0) {
            return g;
        }

        __int128_t s = 0, mn = v[r.begin], mx = v[r.begin];

        for (std::size_t i = r.begin; i < r.end; ++i) {
            s += v[i];

            if (v[i] < mn) {
                mn = v[i];
            }

            if (v[i] > mx) {
                mx = v[i];
            }
        }

        g.sum_mod = red(s);

        if (mode == Mode::exact_extrema) {
            g.minimum = (Value)mn;
            g.maximum = (Value)mx;
        }

        return g;
    }
};

bool same(const Aggregate& x, const Aggregate& y) {
    return x.length == y.length &&
           x.sum_mod == y.sum_mod &&
           x.minimum == y.minimum &&
           x.maximum == y.maximum;
}

bool same_counters(const Counters& x, const Counters& y) {
    return x.nodes_visited == y.nodes_visited &&
           x.push_downs == y.push_downs &&
           x.full_blocks == y.full_blocks &&
           x.partial_elements == y.partial_elements &&
           x.tag_compositions == y.tag_compositions &&
           x.aggregate_merges == y.aggregate_merges;
}

std::vector<Value> make_initial(
    std::mt19937_64& g,
    std::size_t n,
    Mode m
) {
    std::vector<Value> r(n);

    for (auto& x : r) {
        if (m == Mode::exact_extrema) {
            x = (Value)(g() % 2001) - 1000;
        }
        else {
            x = (Value)g();
        }
    }

    return r;
}

AffineTag make_tag(std::mt19937_64& g, Mode m) {
    if (m == Mode::exact_extrema) {
        static const Value gains[] = {-1, 0, 1};

        return {
            gains[g() % 3],
            (Value)(g() % 2001) - 1000
        };
    }

    switch (g() % 6) {
        case 0:
            return {INT64_MIN, INT64_MIN};

        case 1:
            return {INT64_MAX, INT64_MIN};

        case 2:
            return {-1, (Value)g()};

        case 3:
            return {998244353, 998244353LL * 3};

        default:
            return {(Value)g(), (Value)g()};
    }
}

Range make_range(
    std::mt19937_64& g,
    std::size_t n,
    std::size_t b
) {
    std::size_t lo, hi;

    if (g() % 3 == 0 && b > 0) {
        auto pick = [&]() {
            std::size_t k = g() % (n / b + 2);
            std::size_t p = k * b;
            int d = (int)(g() % 3) - 1;
            long long q = (long long)p + d;

            if (q < 0) {
                q = 0;
            }

            if (q > (long long)n) {
                q = (long long)n;
            }

            return (std::size_t)q;
        };

        lo = pick();
        hi = pick();
    }
    else {
        lo = g() % (n + 1);
        hi = g() % (n + 1);
    }

    if (lo > hi) {
        std::swap(lo, hi);
    }

    return {lo, hi};
}

void differential(
    std::uint64_t seed,
    std::size_t n,
    Mode m,
    std::size_t block_override,
    int ops
) {
    set_sqrt_block_size(block_override);

    std::mt19937_64 g(seed);
    auto init = make_initial(g, n, m);

    SqrtDecomposition s;
    s.build(init, m);

    RefArray ref(init, m);

    std::size_t b = block_override;

    if (b == 0) {
        b = 1;

        while (b * b < n) {
            ++b;
        }
    }

    CHECK(s.check_invariants().ok);

    for (int i = 0; i < ops; ++i) {
        Range r = make_range(g, n, b);

        if (g() % 2) {
            AffineTag t = make_tag(g, m);

            s.update(r, t);
            ref.update(r, t);
        }
        else {
            Aggregate got = s.query(r);
            Aggregate want = ref.query(r);

            if (!same(got, want)) {
                std::printf(
                    "MISMATCH seed=%llu n=%zu mode=%d b=%zu op=%d [%zu,%zu)\n",
                    (unsigned long long)seed,
                    n,
                    (int)m,
                    b,
                    i,
                    r.begin,
                    r.end
                );

                ++failures;
                return;
            }
        }

        auto rep = s.check_invariants();

        if (!rep.ok) {
            std::printf(
                "INVARIANT seed=%llu n=%zu op=%d: %s\n",
                (unsigned long long)seed,
                n,
                i,
                rep.violations.empty() ? "?" : rep.violations[0].c_str()
            );

            ++failures;
            return;
        }
    }

    CHECK(same(s.query({0, n}), ref.query({0, n})));

    for (std::size_t i = 0; i < n; ++i) {
        CHECK(same(s.query({i, i + 1}), ref.query({i, i + 1})));
    }

    set_sqrt_block_size(0);
}

void test_contract_examples() {
    set_sqrt_block_size(0);

    for (std::size_t n : {1u, 5u, 9u}) {
        std::vector<Value> init(n, 10);

        SqrtDecomposition a, c;

        a.build(init, Mode::exact_extrema);
        c.build(init, Mode::exact_extrema);

        a.update({0, n}, {1, 3});
        a.update({0, n}, {-1, 0});

        c.update({0, n}, {-1, 0});
        c.update({0, n}, {1, 3});

        CHECK(*a.query({0, n}).minimum == -13 &&
              *a.query({0, n}).maximum == -13);

        CHECK(*c.query({0, n}).minimum == -7 &&
              *c.query({0, n}).maximum == -7);
    }

    SqrtDecomposition s;

    s.build(
        {5, -2, 9, 0, 7, 3, 1},
        Mode::exact_extrema
    );

    s.update({1, 6}, {-1, 0});

    Aggregate q = s.query({0, 7});

    CHECK(*q.minimum == -9 && *q.maximum == 5);

    s.update({0, 7}, {0, 4});

    q = s.query({0, 7});

    CHECK(*q.minimum == 4 &&
          *q.maximum == 4 &&
          q.sum_mod == 28);

    s.update({2, 5}, {1, -10});

    q = s.query({0, 7});

    CHECK(*q.minimum == -6 &&
          *q.maximum == 4 &&
          q.sum_mod == (28 - 30 + 998244353) % 998244353);
}

void test_errors_and_edges() {
    set_sqrt_block_size(0);

    SqrtDecomposition s;

    bool threw = false;

    try {
        s.update({0, 0}, {1, 0});
    }
    catch (const std::logic_error&) {
        threw = true;
    }

    CHECK(threw);

    threw = false;

    try {
        s.query({0, 0});
    }
    catch (const std::logic_error&) {
        threw = true;
    }

    CHECK(threw);

    s.build({}, Mode::exact_extrema);

    CHECK(s.size() == 0);

    Aggregate e = s.query({0, 0});

    CHECK(e.length == 0 &&
          e.sum_mod == 0 &&
          !e.minimum &&
          !e.maximum);

    s.update({0, 0}, {1, 1});

    CHECK(s.check_invariants().ok);

    s.build({1, 2, 3, 4, 5}, Mode::exact_extrema);

    threw = false;

    try {
        s.update({0, 6}, {1, 1});
    }
    catch (const std::out_of_range&) {
        threw = true;
    }

    CHECK(threw);

    threw = false;

    try {
        s.update({3, 2}, {1, 1});
    }
    catch (const std::out_of_range&) {
        threw = true;
    }

    CHECK(threw);

    threw = false;

    try {
        s.query({0, 6});
    }
    catch (const std::out_of_range&) {
        threw = true;
    }

    CHECK(threw);

    threw = false;

    try {
        s.update({0, 5}, {2, 0});
    }
    catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);

    Aggregate q = s.query({0, 5});

    CHECK(*q.minimum == 1 &&
          *q.maximum == 5 &&
          q.sum_mod == 15);

    Aggregate z = s.query({2, 2});

    CHECK(z.length == 0 && !z.minimum);

    s.build({1, 2, 3}, Mode::modular_sum);

    s.update({0, 3}, {INT64_MIN, INT64_MAX});

    q = s.query({0, 3});

    CHECK(!q.minimum && !q.maximum);
    CHECK(q.sum_mod >= 0 && q.sum_mod < 998244353);

    s.build({1}, Mode::exact_extrema);

    CHECK(s.counters().full_blocks == 0 &&
          s.counters().partial_elements == 0);

    s.update({0, 1}, {-1, 0});

    CHECK(s.counters().full_blocks == 1);

    s.reset_counters();

    CHECK(s.counters().full_blocks == 0 &&
          s.counters().tag_compositions == 0);

    s.build({INT64_MIN}, Mode::exact_extrema);

    threw = false;

    try {
        s.update({0, 1}, {-1, 0});
    }
    catch (const std::overflow_error&) {
        threw = true;
    }

    CHECK(threw);
}

void test_counters() {
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

void test_invariant_check_is_read_only() {
    set_sqrt_block_size(0);

    SqrtDecomposition s;

    s.build(
        {1, 2, 3, 4, 5, 6, 7, 8},
        Mode::exact_extrema
    );

    s.update({0, 8}, {1, 10});
    s.update({1, 7}, {-1, 3});
    s.update({2, 6}, {0, 5});

    Aggregate before = s.query({0, 8});

    s.reset_counters();

    auto report = s.check_invariants();

    Counters counters_after_check = s.counters();

    Aggregate after = s.query({0, 8});

    CHECK(report.ok);
    CHECK(same(before, after));
    CHECK(same_counters(Counters{}, counters_after_check));
}

void test_overflow_leaves_block_unchanged() {
    set_sqrt_block_size(2);

    SqrtDecomposition s;

    s.build({INT64_MAX, 0}, Mode::exact_extrema);

    const Aggregate before = s.query({0, 2});

    bool threw = false;

    try {
        s.update({0, 2}, {1, 1});
    }
    catch (const std::overflow_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(same(s.query({0, 2}), before));
    CHECK(s.check_invariants().ok);

    set_sqrt_block_size(0);
}

void test_large_values() {
    set_sqrt_block_size(0);

    const std::vector<Value> exact_values{
        INT64_MAX,
        INT64_MIN,
        17,
        -29
    };

    SqrtDecomposition exact;

    exact.build(exact_values, Mode::exact_extrema);

    RefArray exact_ref(exact_values, Mode::exact_extrema);

    CHECK(same(
        exact.query({0, exact_values.size()}),
        exact_ref.query({0, exact_values.size()})
    ));

    exact.update({1, 4}, {0, INT64_MAX});

    exact_ref.update({1, 4}, {0, INT64_MAX});

    CHECK(same(
        exact.query({0, exact_values.size()}),
        exact_ref.query({0, exact_values.size()})
    ));

    CHECK(exact.check_invariants().ok);

    const std::vector<Value> modular_values{
        INT64_MIN,
        INT64_MAX,
        -1,
        0
    };

    SqrtDecomposition modular;

    modular.build(modular_values, Mode::modular_sum);

    RefArray modular_ref(modular_values, Mode::modular_sum);

    modular.update(
        {0, 4},
        {INT64_MIN, INT64_MAX}
    );

    modular_ref.update(
        {0, 4},
        {INT64_MIN, INT64_MAX}
    );

    CHECK(same(
        modular.query({0, 4}),
        modular_ref.query({0, 4})
    ));

    CHECK(modular.check_invariants().ok);
}

}

int main() {
    test_contract_examples();
    test_errors_and_edges();
    test_counters();
    test_invariant_check_is_read_only();
    test_overflow_leaves_block_unchanged();
    test_large_values();

    int runs = 0;

    for (Mode m : {Mode::exact_extrema, Mode::modular_sum}) {

        for (std::size_t n = 1; n <= 70; ++n) {
            differential(1000 + n, n, m, 0, 300);
            ++runs;
        }

        for (std::size_t b : {1u, 2u, 3u, 7u, 16u, 50u, 1000u}) {
            for (std::size_t n : {
                1u, 2u, 15u, 16u, 17u,
                49u, 50u, 51u, 100u, 257u
            }) {
                differential(77 * b + n, n, m, b, 400);
                ++runs;
            }
        }

        differential(424242, 5000, m, 0, 3000);
        ++runs;
    }

    std::printf(
        "structB_test: %d differential runs, %d failures\n",
        runs,
        failures
    );

    return failures == 0 ? 0 : 1;
}
