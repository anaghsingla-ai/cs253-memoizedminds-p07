#include "cs253/pipeline.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace cs253 {
namespace {
std::size_t automatic_block_size(std::size_t n) {
    if (!n) return 1;
    auto b = static_cast<std::size_t>(std::sqrt(static_cast<long double>(n)));
    if (!b) b = 1;
    while (b < n / b || (b == n / b && n % b)) ++b;
    return b;
}
// A puts ceil(length/2) elements in the left child.
// Follow left children until two leaves remain, then cut between them.
std::size_t deepest_boundary(std::size_t lo, std::size_t hi) {
    while (hi - lo > 2) hi = lo + (hi - lo + 1) / 2;
    return lo + 1;
}
Range maximum_push_range(std::size_t n) {
    if (n < 2) return {0, n};
    if (n == 2) return {0, 1};
    // Put one query boundary in each half so both paths need pushing.
    const auto mid = n / 2 + n % 2;
    return {deepest_boundary(0, mid), n - mid > 1 ? deepest_boundary(mid, n) : n};
}
Range random_width(std::mt19937_64& rng, std::size_t n, std::size_t width) {
    const auto start = static_cast<std::size_t>(rng() % (n - width + 1));
    return {start, start + width};
}
} // namespace

Workload generate(AdversarialTarget target, const GeneratorConfig& config) {
    switch (target) {
        case AdversarialTarget::push_downs:
        case AdversarialTarget::alternating_negative_gain:
        case AdversarialTarget::block_friendly:
        case AdversarialTarget::range_width_sweep: break;
        default: throw std::invalid_argument("unknown adversarial target");
    }
    if (config.mode != Mode::exact_extrema && config.mode != Mode::modular_sum)
        throw std::invalid_argument("unknown generator mode");
    if (config.elements && (!config.range_width || config.range_width > config.elements))
        throw std::invalid_argument("range_width must be in [1,n] for nonempty arrays");
    // |initial|<=1000, |b|<=31, |a|<=1 in exact mode. Bound both
    // logical values and pending offsets, even on leaves never pushed again.
    const auto safe_ops = static_cast<std::uint64_t>(
        (std::numeric_limits<Value>::max() - 1000) / 31);
    if (config.mode == Mode::exact_extrema && config.operations > safe_ops)
        throw std::length_error("operation count exceeds exact arithmetic bound");
    Workload workload;
    workload.mode = config.mode;
    workload.seed = config.seed;
    std::mt19937_64 rng(config.seed);
    workload.initial.resize(config.elements);
    for (auto& x : workload.initial) x = static_cast<Value>(rng() % 2001) - 1000;
    if (config.elements > 1) { workload.initial.front() = -19; workload.initial.back() = 37; }
    workload.operations.reserve(config.operations);
    const auto n = config.elements;
    const auto b = automatic_block_size(n);
    const auto blocks = n ? n / b + (n % b != 0) : 0;
    const auto push_probe = maximum_push_range(n);
    // Widths include points, the whole array, block boundaries and powers of two.
    std::vector<std::size_t> widths;
    if (n) {
        widths = {1, n, config.range_width, b, b > 1 ? b - 1 : 1, std::min(n, b + 1)};
        for (std::size_t x = 1; x < n;) {
            widths.push_back(x);
            if (x > n / 2) break;
            x *= 2;
        }
        std::sort(widths.begin(), widths.end());
        widths.erase(std::unique(widths.begin(), widths.end()), widths.end());
    }
    for (std::size_t i = 0; i < config.operations; ++i) {
        if (!n) { workload.operations.emplace_back(Query{{0, 0}}); continue; }
        const auto phase = i % 16;
        const auto offset = static_cast<Value>(rng() % 63) - 31;
        switch (target) {
            case AdversarialTarget::push_downs:
                // Re-prime before EVERY probe. Resets cannot cancel to identity.
                if (i % 2 == 0) workload.operations.emplace_back(Update{{0, n}, {0, offset}});
                else workload.operations.emplace_back(Query{push_probe});
                break;
            case AdversarialTarget::alternating_negative_gain: {
                const auto mid = n / 2;
                switch (phase) {
                    case 0: workload.operations.emplace_back(Update{{0, n}, {-1, 7}}); break;
                    case 1: workload.operations.emplace_back(Query{{0, n}}); break;
                    case 2: workload.operations.emplace_back(Update{{0, n}, {1, 3}}); break;
                    case 3: workload.operations.emplace_back(Update{{0, n}, {-1, -5}}); break;
                    case 4: workload.operations.emplace_back(Query{{0, 1}}); break;
                    case 5: workload.operations.emplace_back(Update{{mid, n}, {0, -11}}); break;
                    case 6: workload.operations.emplace_back(Update{{0, mid + 1}, {-1, 9}}); break;
                    case 7: workload.operations.emplace_back(Query{{0, mid + 1}}); break;
                    case 8: workload.operations.emplace_back(Update{{0, n}, {-1, 0}}); break;
                    case 9: workload.operations.emplace_back(Query{{0, n}}); break;
                    case 10: workload.operations.emplace_back(Update{{n, n}, {0, 17}}); break;
                    case 11: workload.operations.emplace_back(Query{{n, n}}); break;
                    case 12: workload.operations.emplace_back(Update{{0, n}, {1, -3}}); break;
                    case 13: workload.operations.emplace_back(Update{{0, n}, {-1, 0}}); break;
                    case 14: workload.operations.emplace_back(Query{random_width(rng, n, config.range_width)}); break;
                    default: workload.operations.emplace_back(Query{{n - 1, n}}); break;
                }
                break;
            }
            case AdversarialTarget::block_friendly: {
                // Exactly one aligned block, including the last short block.
                const auto k = (i / 2) % blocks;
                const auto lo = k * b;
                const Range r{lo, lo + std::min(b, n - lo)};
                if (i % 2 == 0) workload.operations.emplace_back(Update{r, {0, offset}});
                else workload.operations.emplace_back(Query{r});
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
                if (i % 2) workload.operations.emplace_back(Query{r});
                else {
                    AffineTag t{static_cast<Value>((i / 2) % 3) - 1, offset};
                    if (config.mode == Mode::modular_sum && i % 6 == 0)
                        t = {std::numeric_limits<Value>::min(), std::numeric_limits<Value>::max()};
                    workload.operations.emplace_back(Update{r, t});
                }
                break;
            }
        }
    }
    return workload;
}
} // namespace cs253
