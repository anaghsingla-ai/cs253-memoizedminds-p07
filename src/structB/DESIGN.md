# Structure B — square-root decomposition with per-block affine tags

Implements `SqrtDecomposition` (include/cs253/sqrt_decomposition.hpp) against INTERFACE.md. Ranges are `[begin,end)`.

## Representation
- `raw[0..n)`: stored elements. Exact mode: int64 values. Modular mode: canonical residues in `[0,P)`, `P = 998244353`.
- Blocks of size `b` (default `ceil(sqrt n)`, override with `set_sqrt_block_size`, `sqrt_config.hpp`); block `k` covers `[k*b, min(n,(k+1)*b))`; the last block may be short and always uses its real length.
- Per block: pending tag `(a,b)` meaning `x -> a*x+b` (identity `(1,0)`), and **logical** aggregates `sum mod P`, `min`, `max` (min/max exact mode only).

## Invariants
1. Logical value of element `i` in block `k` is `a_k*raw[i]+b_k`.
2. Block aggregates describe the logical values, i.e. they already include the pending tag. The tag is owed only to `raw`; it is never re-applied to the aggregates.
3. Exact mode: `a_k in {-1,0,1}`; modular mode: tags, sums and raw are canonical residues.
4. `min <= max` and equal the true extrema of the logical values (exact mode).
`check_invariants()` recomputes (3)-(4) and the sums from `raw` + tag, const, without mutating anything.

## Operations
- **Full-block update** `(a,b)`, length `L`: `sum' = a*sum + b*L (mod P)`; if `a>=0`: `min'=a*min+b, max'=a*max+b` (`a=0` gives `b,b`); if `a<0`: `min'=a*max+b, max'=a*min+b`. Tag: `compose(new, old) = (a*a1, a*b1+b)`. O(1).
- **Partial-block update**: (1) `push_tag`: apply the pending tag to every raw element of the block and reset the tag to `(1,0)`; (2) apply the new tag to the raw elements inside the range; (3) `recompute_block`: rebuild sum/min/max from the raw elements. O(b).
- **Full-block query**: stored `sum/min/max`, O(1).
- **Partial-block query** (read-only, no materialization): compute each overlapped element's real value `a*raw[i]+b` and fold it into the running sum/min/max. O(b).
- A range touches at most 2 partial blocks and `<= n/b` full blocks.

## Complexity
Build O(n). Update and query O(n/b + b) = O(sqrt n) with `b ~ sqrt n`. Space: n values + n/b blocks.

## Arithmetic and error behaviour
- Exact: 128-bit intermediates, checked narrowing to int64; overflow throws `std::overflow_error` (outside supported workload). A full-block overflow throws before mutating that block, but an overflow in the middle of a multi-block update or inside a partial pass can leave the structure inconsistent: rebuild after such an exception.
- Modular: `a`, `b`, initial values reduced mod P first (also INT64_MIN); negative intermediates normalised.
- Invalid range -> `std::out_of_range`, exact gain not in {-1,0,1} -> `std::invalid_argument`, both before mutation; use before `build` -> `std::logic_error`.

## Counters (units)
`full_blocks`: full blocks processed (update and query). `partial_elements`: raw entries touched in partial processing (update: the whole block, counted once per entry, for the push/update/recompute work; query: overlapped entries). `tag_compositions`: composition calls (one per full-block update). `aggregate_merges`: folds of block results into a query answer after the first. `nodes_visited`, `push_downs`: always 0 (no tree, no push-down).
