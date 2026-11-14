#include "cs253/pipeline.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace cs253 {
namespace {
std::size_t block_size(std::size_t n) {
    if (!n) return 1;
    auto b = static_cast<std::size_t>(std::sqrt(static_cast<long double>(n)));
    if (!b) b = 1;
    while (b < n / b || (b == n / b && n % b)) ++b;
    return b;
}
// A splits with ceil(length/2) leaves on the left. Cut a deepest pair.
std::size_t deep_cut(std::size_t lo, std::size_t hi) {
    while (hi - lo > 2) hi = lo + (hi - lo + 1) / 2;
    return lo + 1;
}
Range push_range(std::size_t n) {
    if (n < 2) return {0, n};
    if (n == 2) return {0, 1};
    const auto mid = n / 2 + n % 2;
    return {deep_cut(0, mid), n - mid > 1 ? deep_cut(mid, n) : n};
}
Range random_width(std::mt19937_64& rng, std::size_t n, std::size_t width) {
    const auto start = static_cast<std::size_t>(rng() % (n - width + 1));
    return {start, start + width};
}
} // namespace

Workload generate(AdversarialTarget target, const GeneratorConfig& c) {
    switch (target) {
        case AdversarialTarget::push_downs:
        case AdversarialTarget::alternating_negative_gain:
        case AdversarialTarget::block_friendly:
        case AdversarialTarget::range_width_sweep: break;
        default: throw std::invalid_argument("unknown adversarial target");
    }
    if (c.mode != Mode::exact_extrema && c.mode != Mode::modular_sum)
        throw std::invalid_argument("unknown generator mode");
    if (c.elements && (!c.range_width || c.range_width > c.elements))
        throw std::invalid_argument("range_width must be in [1,n] for nonempty arrays");
    // |initial|<=1000, |b|<=31, |a|<=1 in exact mode. Bound both
    // logical values and pending offsets, even on leaves never pushed again.
    const auto safe_ops = static_cast<std::uint64_t>(
        (std::numeric_limits<Value>::max() - 1000) / 31);
    if (c.mode == Mode::exact_extrema && c.operations > safe_ops)
        throw std::length_error("operation count exceeds exact arithmetic bound");
    Workload w;
    w.mode = c.mode; w.seed = c.seed;
    std::mt19937_64 rng(c.seed);
    w.initial.resize(c.elements);
    for (auto& x : w.initial) x = static_cast<Value>(rng() % 2001) - 1000;
    if (c.elements > 1) { w.initial.front() = -19; w.initial.back() = 37; }
    w.operations.reserve(c.operations);
    const auto n = c.elements;
    const auto b = block_size(n);
    const auto blocks = n ? n / b + (n % b != 0) : 0;
    const auto push_probe = push_range(n);
    std::vector<std::size_t> widths;
    if (n) {
        widths = {1, n, c.range_width, b, b > 1 ? b - 1 : 1, std::min(n, b + 1)};
        for (std::size_t x = 1; x < n;) {
            widths.push_back(x);
            if (x > n / 2) break;
            x *= 2;
        }
        std::sort(widths.begin(), widths.end());
        widths.erase(std::unique(widths.begin(), widths.end()), widths.end());
    }
    for (std::size_t i = 0; i < c.operations; ++i) {
        if (!n) { w.operations.emplace_back(Query{{0, 0}}); continue; }
        const auto phase = i % 16;
        const auto offset = static_cast<Value>(rng() % 63) - 31;
        switch (target) {
            case AdversarialTarget::push_downs:
                // Re-prime before EVERY probe. Resets cannot cancel to identity.
                if (i % 2 == 0) w.operations.emplace_back(Update{{0, n}, {0, offset}});
                else w.operations.emplace_back(Query{push_probe});
                break;
            case AdversarialTarget::alternating_negative_gain: {
                const auto mid = n / 2;
                switch (phase) {
                    case 0: w.operations.emplace_back(Update{{0, n}, {-1, 7}}); break;
                    case 1: w.operations.emplace_back(Query{{0, n}}); break;
                    case 2: w.operations.emplace_back(Update{{0, n}, {1, 3}}); break;
                    case 3: w.operations.emplace_back(Update{{0, n}, {-1, -5}}); break;
                    case 4: w.operations.emplace_back(Query{{0, 1}}); break;
                    case 5: w.operations.emplace_back(Update{{mid, n}, {0, -11}}); break;
                    case 6: w.operations.emplace_back(Update{{0, mid + 1}, {-1, 9}}); break;
                    case 7: w.operations.emplace_back(Query{{0, mid + 1}}); break;
                    case 8: w.operations.emplace_back(Update{{0, n}, {-1, 0}}); break;
                    case 9: w.operations.emplace_back(Query{{0, n}}); break;
                    case 10: w.operations.emplace_back(Update{{n, n}, {0, 17}}); break;
                    case 11: w.operations.emplace_back(Query{{n, n}}); break;
                    case 12: w.operations.emplace_back(Update{{0, n}, {1, -3}}); break;
                    case 13: w.operations.emplace_back(Update{{0, n}, {-1, 0}}); break;
                    case 14: w.operations.emplace_back(Query{random_width(rng, n, c.range_width)}); break;
                    default: w.operations.emplace_back(Query{{n - 1, n}}); break;
                }
                break;
            }
            case AdversarialTarget::block_friendly: {
                // Exactly one aligned block, including the last short block.
                const auto k = (i / 2) % blocks;
                const auto lo = k * b;
                const Range r{lo, lo + std::min(b, n - lo)};
                if (i % 2 == 0) w.operations.emplace_back(Update{r, {0, offset}});
                else w.operations.emplace_back(Query{r});
                break;
            }
            case AdversarialTarget::range_width_sweep: {
                const auto width = widths[(i / 8) % widths.size()];
                Range r;
                switch ((i / 2) % 4) {
                    case 0: r = {0, width}; break;
                    case 1: r = {n - width, n}; break;
                    case 2: { const std::size_t lo = width < n ? 1 : 0; r = {lo, lo + width}; break; }
                    default: r = random_width(rng, n, width); break;
                }
                if (i % 2) w.operations.emplace_back(Query{r});
                else {
                    AffineTag t{static_cast<Value>((i / 2) % 3) - 1, offset};
                    if (c.mode == Mode::modular_sum && i % 6 == 0)
                        t = {std::numeric_limits<Value>::min(), std::numeric_limits<Value>::max()};
                    w.operations.emplace_back(Update{r, t});
                }
                break;
            }
        }
    }
    return w;
}
} // namespace cs253
