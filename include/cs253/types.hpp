#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>
namespace cs253 {
using Value = std::int64_t;
using Residue = std::int64_t;
using Seed = std::uint64_t;
inline constexpr Residue modulus = 998244353;
enum class Mode { exact_extrema, modular_sum };

struct Range { std::size_t begin = 0, end = 0; };

struct AffineTag { Value a = 1, b = 0; };
struct Aggregate {
    std::size_t length = 0;
    Residue sum_mod = 0;
    std::optional<Value> minimum, maximum;
};
struct Update { Range range; AffineTag tag; };
struct Query { Range range; };
using Operation = std::variant<Update, Query>;
struct Workload {
    Mode mode = Mode::exact_extrema;
    Seed seed = 0;
    std::vector<Value> initial;
    std::vector<Operation> operations;
};
struct Counters {
    std::uint64_t nodes_visited = 0, push_downs = 0;
    std::uint64_t full_blocks = 0, partial_elements = 0;
    std::uint64_t tag_compositions = 0, aggregate_merges = 0;
};
struct InvariantReport { bool ok = false; std::vector<std::string> violations; };
}
