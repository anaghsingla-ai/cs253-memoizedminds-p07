// Adversarial workload generators. Tool assistance is declared in TOOLS.md.
#include "cs253/pipeline.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace cs253 {
namespace {

// Add one instruction. The structures execute it later.
void add_update(Workload& workload, std::size_t begin, std::size_t end,
                Value gain, Value offset) {
    Update update;
    update.range.begin = begin;
    update.range.end = end;
    update.tag.a = gain;
    update.tag.b = offset;
    workload.operations.push_back(update);
}

void add_query(Workload& workload, std::size_t begin, std::size_t end) {
    Query query;
    query.range.begin = begin;
    query.range.end = end;
    workload.operations.push_back(query);
}

// Return ceil(sqrt(n)). Division avoids overflowing when squaring a large size.
std::size_t automatic_block_size(std::size_t n) {
    if (n == 0) {
        return 1;
    }

    long double root = std::sqrt(static_cast<long double>(n));
    std::size_t block_size = static_cast<std::size_t>(root);
    if (block_size == 0) {
        block_size = 1;
    }

    while (block_size < n / block_size) {
        block_size++;
    }
    if (block_size == n / block_size && n % block_size != 0) {
        block_size++;
    }
    return block_size;
}

// Follow left children until two leaves remain, then cut between them.
std::size_t deepest_boundary(std::size_t lo, std::size_t hi) {
    while (hi - lo > 2) {
        std::size_t length = hi - lo;
        std::size_t left_length = length / 2;
        if (length % 2 != 0) {
            left_length++;
        }
        hi = lo + left_length;
    }
    return lo + 1;
}

Range maximum_push_range(std::size_t n) {
    Range range;
    if (n < 2) {
        range.begin = 0;
        range.end = n;
    } else if (n == 2) {
        range.begin = 0;
        range.end = 1;
    } else {
        std::size_t middle = n / 2;
        if (n % 2 != 0) {
            middle++;
        }

        // Put one boundary deep in each half.
        range.begin = deepest_boundary(0, middle);
        if (n - middle > 1) {
            range.end = deepest_boundary(middle, n);
        } else {
            range.end = n;
        }
    }
    return range;
}

Range random_width(std::mt19937_64& rng, std::size_t n, std::size_t width) {
    std::size_t possible_starts = n - width + 1;
    std::size_t start = static_cast<std::size_t>(rng() % possible_starts);

    Range range;
    range.begin = start;
    range.end = start + width;
    return range;
}

void check_config(AdversarialTarget target, const GeneratorConfig& config) {
    if (target != AdversarialTarget::push_downs &&
        target != AdversarialTarget::alternating_negative_gain &&
        target != AdversarialTarget::block_friendly &&
        target != AdversarialTarget::range_width_sweep) {
        throw std::invalid_argument("unknown adversarial target");
    }
    if (config.mode != Mode::exact_extrema &&
        config.mode != Mode::modular_sum) {
        throw std::invalid_argument("unknown generator mode");
    }

    if (config.elements > 0) {
        if (config.range_width == 0 || config.range_width > config.elements) {
            throw std::invalid_argument("range_width must be in [1,n] for nonempty arrays");
        }
    }

    // Exact gains are -1, 0 or 1. Initial |x| <= 1000 and every |offset| <= 31.
    // This also bounds offsets that accumulate in pending tags.
    Value largest_value = std::numeric_limits<Value>::max();
    std::uint64_t safe_operations =
        static_cast<std::uint64_t>((largest_value - 1000) / 31);
    if (config.mode == Mode::exact_extrema &&
        config.operations > safe_operations) {
        throw std::length_error("operation count exceeds exact arithmetic bound");
    }
}

std::vector<std::size_t> make_widths(std::size_t n, std::size_t requested_width,
                                    std::size_t block_size) {
    std::vector<std::size_t> widths;
    if (n == 0) {
        return widths;
    }

    widths.push_back(1);
    widths.push_back(n);
    widths.push_back(requested_width);
    widths.push_back(block_size);
    if (block_size > 1) {
        widths.push_back(block_size - 1);
    } else {
        widths.push_back(1);
    }
    if (block_size < n) {
        widths.push_back(block_size + 1);
    } else {
        widths.push_back(n);
    }

    std::size_t width = 1;
    while (width < n) {
        widths.push_back(width);
        if (width > n / 2) {
            break;
        }
        width = width * 2;
    }

    std::sort(widths.begin(), widths.end());

    // Remove repeated widths with a normal loop.
    std::vector<std::size_t> different_widths;
    for (std::size_t i = 0; i < widths.size(); i++) {
        if (different_widths.empty()) {
            different_widths.push_back(widths[i]);
        } else if (different_widths.back() != widths[i]) {
            different_widths.push_back(widths[i]);
        }
    }
    return different_widths;
}

void add_negative_operation(Workload& workload, std::size_t phase,
                            std::size_t n, std::size_t width,
                            std::mt19937_64& rng) {
    std::size_t middle = n / 2;

    if (phase == 0) {
        add_update(workload, 0, n, -1, 7);
    } else if (phase == 1) {
        // Query before another negation could hide a missing extrema swap.
        add_query(workload, 0, n);
    } else if (phase == 2) {
        add_update(workload, 0, n, 1, 3);
    } else if (phase == 3) {
        add_update(workload, 0, n, -1, -5);
    } else if (phase == 4) {
        add_query(workload, 0, 1);
    } else if (phase == 5) {
        add_update(workload, middle, n, 0, -11);
    } else if (phase == 6) {
        add_update(workload, 0, middle + 1, -1, 9);
    } else if (phase == 7) {
        add_query(workload, 0, middle + 1);
    } else if (phase == 8) {
        add_update(workload, 0, n, -1, 0);
    } else if (phase == 9) {
        add_query(workload, 0, n);
    } else if (phase == 10) {
        add_update(workload, n, n, 0, 17);
    } else if (phase == 11) {
        add_query(workload, n, n);
    } else if (phase == 12) {
        add_update(workload, 0, n, 1, -3);
    } else if (phase == 13) {
        add_update(workload, 0, n, -1, 0);
    } else if (phase == 14) {
        Range range = random_width(rng, n, width);
        add_query(workload, range.begin, range.end);
    } else {
        add_query(workload, n - 1, n);
    }
}

} // namespace

Workload generate(AdversarialTarget target, const GeneratorConfig& config) {
    check_config(target, config);

    Workload workload;
    workload.mode = config.mode;
    workload.seed = config.seed;

    std::mt19937_64 rng(config.seed);
    workload.initial.resize(config.elements);
    for (std::size_t i = 0; i < config.elements; i++) {
        Value value = static_cast<Value>(rng() % 2001);
        workload.initial[i] = value - 1000;
    }
    if (config.elements > 1) {
        workload.initial.front() = -19;
        workload.initial.back() = 37;
    }
    workload.operations.reserve(config.operations);

    std::size_t n = config.elements;
    std::size_t block_size = automatic_block_size(n);
    std::size_t block_count = 0;
    if (n > 0) {
        block_count = n / block_size;
        if (n % block_size != 0) {
            block_count++;
        }
    }

    Range push_probe = maximum_push_range(n);
    std::vector<std::size_t> widths =
        make_widths(n, config.range_width, block_size);

    for (std::size_t i = 0; i < config.operations; i++) {
        if (n == 0) {
            add_query(workload, 0, 0);
            continue;
        }

        // Keep one random draw per nonempty operation, even if this target
        // uses fixed corrections. This preserves earlier seeded workloads.
        Value offset = static_cast<Value>(rng() % 63) - 31;

        if (target == AdversarialTarget::push_downs) {
            if (i % 2 == 0) {
                // A reset is always nonidentity, including a reset to zero.
                add_update(workload, 0, n, 0, offset);
            } else {
                add_query(workload, push_probe.begin, push_probe.end);
            }
        } else if (target == AdversarialTarget::alternating_negative_gain) {
            std::size_t phase = i % 16;
            add_negative_operation(workload, phase, n, config.range_width, rng);
        } else if (target == AdversarialTarget::block_friendly) {
            std::size_t block = (i / 2) % block_count;
            std::size_t begin = block * block_size;
            std::size_t length = block_size;
            if (n - begin < length) {
                length = n - begin;
            }
            std::size_t end = begin + length;

            if (i % 2 == 0) {
                add_update(workload, begin, end, 0, offset);
            } else {
                add_query(workload, begin, end);
            }
        } else if (target == AdversarialTarget::range_width_sweep) {
            std::size_t width_index = (i / 8) % widths.size();
            std::size_t width = widths[width_index];
            std::size_t position = (i / 2) % 4;
            Range range;

            if (position == 0) {
                range.begin = 0;
                range.end = width;
            } else if (position == 1) {
                range.begin = n - width;
                range.end = n;
            } else if (position == 2) {
                range.begin = 0;
                if (width < n) {
                    range.begin = 1;
                }
                range.end = range.begin + width;
            } else {
                range = random_width(rng, n, width);
            }

            if (i % 2 != 0) {
                add_query(workload, range.begin, range.end);
            } else {
                Value gain = static_cast<Value>((i / 2) % 3) - 1;
                Value update_offset = offset;
                if (config.mode == Mode::modular_sum && i % 6 == 0) {
                    gain = std::numeric_limits<Value>::min();
                    update_offset = std::numeric_limits<Value>::max();
                }
                add_update(workload, range.begin, range.end, gain, update_offset);
            }
        }
    }
    return workload;
}

} // namespace cs253
