#include "cs253/pipeline.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/sqrt_config.hpp"
#include "../adversary/export.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace cs253;
namespace {
void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}
constexpr AdversarialTarget targets[] = {AdversarialTarget::push_downs,
    AdversarialTarget::alternating_negative_gain, AdversarialTarget::block_friendly,
    AdversarialTarget::range_width_sweep};
bool same(const Aggregate& a, const Aggregate& b) {
    return a.length == b.length && a.sum_mod == b.sum_mod &&
           a.minimum == b.minimum && a.maximum == b.maximum;
}
// Independent test-only array, not the Oracle owner's implementation.
struct Reference {
    std::vector<Value> values;
    Mode mode;
    static Residue reduce(__int128_t x) {
        const auto r = static_cast<Residue>(x % 998244353);
        return r < 0 ? r + 998244353 : r;
    }
    explicit Reference(const Workload& w) : values(w.initial), mode(w.mode) {
        if (mode == Mode::modular_sum) for (auto& x : values) x = reduce(x);
    }
    void update(const Update& u) {
        const __int128_t a = mode == Mode::modular_sum ? reduce(u.tag.a) : u.tag.a;
        const __int128_t b = mode == Mode::modular_sum ? reduce(u.tag.b) : u.tag.b;
        for (auto i = u.range.begin; i < u.range.end; ++i) {
            const auto y = a * values[i] + b;
            if (mode == Mode::exact_extrema) {
                require(y >= std::numeric_limits<Value>::min() && y <= std::numeric_limits<Value>::max(), "exact overflow");
                values[i] = static_cast<Value>(y);
            } else values[i] = reduce(y);
        }
    }
    Aggregate query(Range r) const {
        Aggregate out;
        out.length = r.end - r.begin;
        __int128_t sum = 0;
        for (auto i = r.begin; i < r.end; ++i) {
            sum += values[i];
            if (mode == Mode::exact_extrema) {
                if (!out.minimum || values[i] < *out.minimum) out.minimum = values[i];
                if (!out.maximum || values[i] > *out.maximum) out.maximum = values[i];
            }
        }
        out.sum_mod = reduce(sum);
        return out;
    }
};
void validate(const Workload& w, const GeneratorConfig& c) {
    require(w.initial.size() == c.elements && w.operations.size() == c.operations, "wrong size");
    require(w.seed == c.seed && w.mode == c.mode, "wrong metadata");
    for (const auto& op : w.operations) {
        const auto* u = std::get_if<Update>(&op);
        const auto r = u ? u->range : std::get<Query>(op).range;
        require(r.begin <= r.end && r.end <= c.elements, "invalid generated range");
        if (u && c.mode == Mode::exact_extrema)
            require(u->tag.a >= -1 && u->tag.a <= 1, "invalid exact gain");
    }
}
void differential(const Workload& w) {
    LazySegmentTree a;
    SqrtDecomposition b;
    a.build(w.initial, w.mode); b.build(w.initial, w.mode);
    Reference ref(w);
    std::size_t index = 0;
    try {
        for (const auto& op : w.operations) {
            if (const auto* u = std::get_if<Update>(&op)) {
                a.update(u->range, u->tag); b.update(u->range, u->tag); ref.update(*u);
            } else {
                const auto r = std::get<Query>(op).range;
                const auto expected = ref.query(r);
                require(same(a.query(r), expected), "A query mismatch");
                require(same(b.query(r), expected), "B query mismatch");
            }
            // Read-only audits preserve the pending-tag state for the next op.
            require(a.check_invariants().ok, "A invariant failure");
            require(b.check_invariants().ok, "B invariant failure");
            ++index;
        }
        const auto expected = ref.query({0, w.initial.size()});
        require(same(a.query({0, w.initial.size()}), expected), "A final mismatch");
        require(same(b.query({0, w.initial.size()}), expected), "B final mismatch");
    } catch (const std::exception& e) {
        std::ofstream out("adversary_failure.txt");
        adversary::export_workload(out, w);
        std::cerr << "seed=" << w.seed << " operation=" << index << ": " << e.what()
                  << "; workload saved to adversary_failure.txt\n";
        throw;
    }
}
void exact_push_counts() {
    // Exhaustive search over ALL ranges independently checks maximality on
    // small uneven sizes as well as the 2h-1 closed form for powers of two.
    for (std::size_t n = 1; n <= 48; ++n) {
        GeneratorConfig c; c.elements = n; c.operations = 2;
        const auto w = generate(AdversarialTarget::push_downs, c);
        LazySegmentTree a;
        a.build(w.initial, c.mode);
        a.update({0, n}, {0, 7});
        a.reset_counters();
        a.query(std::get<Query>(w.operations[1]).range);
        const auto got = a.counters().push_downs;
        std::uint64_t best = 0;
        for (std::size_t lo = 0; lo <= n; ++lo)
            for (std::size_t hi = lo; hi <= n; ++hi) {
                a.update({0, n}, {0, 7}); a.reset_counters(); a.query({lo, hi});
                best = std::max(best, a.counters().push_downs);
            }
        require(got == best, "push range not maximal for n=" + std::to_string(n));
    }
    for (unsigned h = 0; h <= 14; ++h) {
        const std::size_t n = std::size_t{1} << h;
        GeneratorConfig c; c.elements = n; c.operations = 10;
        const auto w = generate(AdversarialTarget::push_downs, c);
        LazySegmentTree a; a.build(w.initial, c.mode);
        for (const auto& op : w.operations) {
            if (const auto* u = std::get_if<Update>(&op)) a.update(u->range, u->tag);
            else {
                a.reset_counters(); a.query(std::get<Query>(op).range);
                require(a.counters().push_downs == (h ? 2 * h - 1 : 0), "wrong exact push bound");
            }
        }
    }
}
void block_counts() {
    for (std::size_t n : {1u, 2u, 17u, 49u, 50u, 64u, 1000u}) {
        GeneratorConfig c; c.elements = n; c.operations = 128;
        const auto w = generate(AdversarialTarget::block_friendly, c);
        SqrtDecomposition b; b.build(w.initial, c.mode);
        for (const auto& op : w.operations) {
            b.reset_counters();
            if (const auto* u = std::get_if<Update>(&op)) b.update(u->range, u->tag);
            else b.query(std::get<Query>(op).range);
            require(b.counters().full_blocks == 1 && b.counters().partial_elements == 0,
                    "block-friendly operation did partial work");
        }
    }
}
void scale() {
    for (auto target : targets) for (auto mode : {Mode::exact_extrema, Mode::modular_sum}) {
        GeneratorConfig c; c.mode = mode; c.range_width = 999;
        const auto w = generate(target, c);
        validate(w, c);
        // Full-scale generation checks, not an unclaimed full-scale benchmark.
        require(w.initial.size() == 1000000 && w.operations.size() == 1000000, "scale mismatch");
    }
}
void errors() {
    const auto rejected = [](AdversarialTarget t, GeneratorConfig c) {
        bool threw = false;
        try { generate(t, c); } catch (const std::exception&) { threw = true; }
        require(threw, "invalid config accepted");
    };
    GeneratorConfig c; c.elements = 5; c.operations = 1;
    c.range_width = 0; rejected(targets[0], c);
    c.range_width = 6; rejected(targets[0], c);
    c.range_width = 1; rejected(static_cast<AdversarialTarget>(99), c);
    c.mode = static_cast<Mode>(99); rejected(targets[0], c);
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--scale") {
            scale(); std::cout << "8 million-operation generation cases passed\n"; return 0;
        }
        set_sqrt_block_size(0);
        std::size_t runs = 0;
        for (auto target : targets) for (auto mode : {Mode::exact_extrema, Mode::modular_sum})
            for (auto n : {0u, 1u, 2u, 3u, 5u, 16u, 17u, 49u, 50u, 257u})
                for (Seed seed : {0ULL, 253ULL, 18446744073709551615ULL}) {
                    GeneratorConfig c; c.elements = n; c.operations = 512; c.seed = seed;
                    c.mode = mode; c.range_width = n ? std::min<std::size_t>(n, 7) : 0;
                    const auto w = generate(target, c);
                    validate(w, c);
                    std::ostringstream first, second;
                    adversary::export_workload(first, w);
                    adversary::export_workload(second, generate(target, c));
                    require(first.str() == second.str(), "non-reproducible output");
                    differential(w); ++runs;
                }
        exact_push_counts(); block_counts(); errors();
        GeneratorConfig c; c.elements = 257; c.operations = 512; c.range_width = 7;
        auto w = generate(AdversarialTarget::range_width_sweep, c);
        bool single = false, whole = false, configured = false;
        for (const auto& op : w.operations) {
            const auto* u = std::get_if<Update>(&op);
            const auto r = u ? u->range : std::get<Query>(op).range;
            single |= r.end - r.begin == 1; whole |= r.end - r.begin == 257;
            configured |= r.end - r.begin == 7;
        }
        require(single && whole && configured, "width sweep missed required widths");
        for (auto target : targets) {
            c.operations = 0; require(generate(target, c).operations.empty(), "q=0 failed");
        }
        std::cout << runs << " differential/reproducibility cases; push-down maxima and block counters passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "adversary_test: " << e.what() << '\n'; return 1;
    }
}
