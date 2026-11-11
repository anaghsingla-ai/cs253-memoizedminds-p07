#pragma once
#include <cstddef>
namespace cs253 {
// Structure B tuning knob (additive; does not change the shared contract).
// Block size used by the NEXT SqrtDecomposition::build. 0 (default) = automatic,
// ceil(sqrt(n)), at least 1. Not thread-safe; set it before building.
void set_sqrt_block_size(std::size_t block_size) noexcept;
std::size_t sqrt_block_size_setting() noexcept;
} // namespace cs253
