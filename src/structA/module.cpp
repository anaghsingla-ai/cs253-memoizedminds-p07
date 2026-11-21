#include "cs253/lazy_segment_tree.hpp"
#include <algorithm>
#include <climits>
#include <limits>
#include <stdexcept>
#include <string>

namespace cs253 {

namespace {

using Wide = __int128_t;


Residue norm(Wide x) {
    x %= modulus;
    if (x < 0) x += modulus;
    return static_cast<Residue>(x);
}


Value checked(Wide x) {
    if (x < std::numeric_limits<Value>::min() ||
        x > std::numeric_limits<Value>::max()) {
        throw std::overflow_error(
            "exact affine arithmetic overflow"
        );
    }
    return static_cast<Value>(x);
}

bool identity(AffineTag t) {
    return t.a == 1 && t.b == 0;
}





AffineTag compose(AffineTag newer,
                  AffineTag older,
                  Mode mode) {
    if (mode == Mode::modular_sum) {
        return {
            norm(Wide(newer.a) * older.a),
            norm(Wide(newer.a) * older.b + newer.b)
        };
    }

    return {
        checked(Wide(newer.a) * older.a),
        checked(Wide(newer.a) * older.b + newer.b)
    };
}


Aggregate merge_aggregates(
    const Aggregate& left,
    const Aggregate& right
) {
    if (left.length == 0) return right;
    if (right.length == 0) return left;

    Aggregate out;

    out.length = left.length + right.length;
    out.sum_mod = norm(
        Wide(left.sum_mod) + right.sum_mod
    );

    if (left.minimum && right.minimum) {
        out.minimum = std::min(
            *left.minimum, *right.minimum
        );
        out.maximum = std::max(
            *left.maximum, *right.maximum
        );
    }

    return out;
}

}






void LazySegmentTree::build(
    const std::vector<Value>& initial,
    Mode requested_mode
) {
    if (requested_mode != Mode::modular_sum &&
        requested_mode != Mode::exact_extrema) {
        throw std::invalid_argument("LazySegmentTree: unsupported mode");
    }

    if (initial.size() >
        static_cast<std::size_t>(INT_MAX)) {
        throw std::length_error(
            "tree too large for int-based recursion"
        );
    }

    built = false;
    n = initial.size();
    current_mode = requested_mode;

    tree.assign(4 * n + 4, Node{});
    stats = {};

    if (n > 0) {
        build_tree(
            1, 0, static_cast<int>(n) - 1, initial
        );
    }


    stats = {};
    built = true;
}

void LazySegmentTree::build_tree(
    int v, int tl, int tr,
    const std::vector<Value>& a
) {
    if (tl == tr) {
        tree[v].sum = norm(a[tl]);

        if (current_mode == Mode::exact_extrema) {
            tree[v].mn = a[tl];
            tree[v].mx = a[tl];
        }

        return;
    }

    int tm = tl + (tr - tl) / 2;

    build_tree(2 * v, tl, tm, a);
    build_tree(2 * v + 1, tm + 1, tr, a);

    pull(v);
}






std::size_t LazySegmentTree::size() const {
    if (!built) throw std::logic_error("LazySegmentTree: build() has not been called");
    return n;
}

Mode LazySegmentTree::mode() const {
    if (!built) throw std::logic_error("LazySegmentTree: build() has not been called");
    return current_mode;
}

Counters LazySegmentTree::counters() const noexcept {
    return stats;
}

void LazySegmentTree::reset_counters() noexcept {
    stats = {};
}






void LazySegmentTree::pull(int v) {
    tree[v].sum = norm(
        Wide(tree[2 * v].sum) +
        tree[2 * v + 1].sum
    );

    if (current_mode == Mode::exact_extrema) {
        tree[v].mn = std::min(
            tree[2 * v].mn,
            tree[2 * v + 1].mn
        );

        tree[v].mx = std::max(
            tree[2 * v].mx,
            tree[2 * v + 1].mx
        );
    }

    ++stats.aggregate_merges;
}






void LazySegmentTree::apply(
    int v, int tl, int tr, AffineTag tag
) {
    if (current_mode == Mode::exact_extrema &&
        tag.a != -1 && tag.a != 0 && tag.a != 1) {
        throw std::invalid_argument(
            "exact_extrema requires gain -1, 0, or 1"
        );
    }



    Residue new_sum = norm(
        Wide(norm(tag.a)) * tree[v].sum +
        Wide(norm(tag.b)) * (tr - tl + 1)
    );

    Value new_min = tree[v].mn;
    Value new_max = tree[v].mx;

    if (current_mode == Mode::exact_extrema) {
        if (tag.a == 1) {

            new_min = checked(
                Wide(tree[v].mn) + tag.b
            );
            new_max = checked(
                Wide(tree[v].mx) + tag.b
            );
        }
        else if (tag.a == -1) {


            new_min = checked(
                -Wide(tree[v].mx) + tag.b
            );
            new_max = checked(
                -Wide(tree[v].mn) + tag.b
            );
        }
        else {

            new_min = tag.b;
            new_max = tag.b;
        }
    }


    AffineTag new_lazy = compose(
        tag, tree[v].lazy, current_mode
    );

    tree[v].sum = new_sum;

    if (current_mode == Mode::exact_extrema) {
        tree[v].mn = new_min;
        tree[v].mx = new_max;
    }

    tree[v].lazy = new_lazy;
    ++stats.tag_compositions;
}






void LazySegmentTree::push(
    int v, int tl, int tr
) {
    if (tl == tr || identity(tree[v].lazy)) {
        return;
    }

    int tm = tl + (tr - tl) / 2;
    AffineTag tag = tree[v].lazy;

    apply(2 * v, tl, tm, tag);
    apply(2 * v + 1, tm + 1, tr, tag);


    tree[v].lazy = {1, 0};

    ++stats.push_downs;
}






void LazySegmentTree::update_tree(
    int v, int tl, int tr,
    int l, int r, AffineTag tag
) {
    ++stats.nodes_visited;


    if (l <= tl && tr <= r) {
        apply(v, tl, tr, tag);
        return;
    }

    push(v, tl, tr);

    int tm = tl + (tr - tl) / 2;

    if (l <= tm) {
        update_tree(
            2 * v, tl, tm, l, r, tag
        );
    }

    if (r > tm) {
        update_tree(
            2 * v + 1, tm + 1, tr, l, r, tag
        );
    }

    pull(v);
}






Aggregate LazySegmentTree::query_tree(
    int v, int tl, int tr,
    int l, int r
) {
    ++stats.nodes_visited;


    if (l <= tl && tr <= r) {
        Aggregate out;

        out.length = static_cast<std::size_t>(
            tr - tl + 1
        );

        out.sum_mod = tree[v].sum;

        if (current_mode == Mode::exact_extrema) {
            out.minimum = tree[v].mn;
            out.maximum = tree[v].mx;
        }

        return out;
    }

    push(v, tl, tr);

    int tm = tl + (tr - tl) / 2;

    if (r <= tm) {
        return query_tree(
            2 * v, tl, tm, l, r
        );
    }

    if (l > tm) {
        return query_tree(
            2 * v + 1, tm + 1, tr, l, r
        );
    }

    Aggregate left = query_tree(
        2 * v, tl, tm, l, r
    );

    Aggregate right = query_tree(
        2 * v + 1, tm + 1, tr, l, r
    );

    ++stats.aggregate_merges;

    return merge_aggregates(left, right);
}






void LazySegmentTree::update(
    Range range, AffineTag tag
) {
    if (!built) throw std::logic_error("LazySegmentTree: build() has not been called");

    if (range.begin > range.end ||
        range.end > n) {
        throw std::out_of_range(
            "invalid update range"
        );
    }

    if (current_mode == Mode::exact_extrema &&
        tag.a != -1 && tag.a != 0 && tag.a != 1) {
        throw std::invalid_argument(
            "unsupported exact_extrema gain"
        );
    }


    if (range.begin == range.end) return;

    if (current_mode == Mode::modular_sum) {
        tag.a = norm(tag.a);
        tag.b = norm(tag.b);
    }


    update_tree(
        1,
        0,
        static_cast<int>(n) - 1,
        static_cast<int>(range.begin),
        static_cast<int>(range.end) - 1,
        tag
    );
}






Aggregate LazySegmentTree::query(Range range) {
    if (!built) throw std::logic_error("LazySegmentTree: build() has not been called");

    if (range.begin > range.end ||
        range.end > n) {
        throw std::out_of_range(
            "invalid query range"
        );
    }

    if (range.begin == range.end) {
        return {};
    }

    return query_tree(
        1,
        0,
        static_cast<int>(n) - 1,
        static_cast<int>(range.begin),
        static_cast<int>(range.end) - 1
    );
}






InvariantReport LazySegmentTree::check_invariants() const {
    InvariantReport report{true, {}};

    if (tree.size() != 4 * n + 4) {
        report.ok = false;
        report.violations.push_back(
            "tree storage size mismatch"
        );
        return report;
    }

    if (n == 0) return report;


    auto inspect = [&](auto&& self,
                       int v, int tl, int tr) -> void {
        const Node& node = tree[v];

        if (node.sum < 0 || node.sum >= modulus) {
            report.ok = false;
            report.violations.push_back(
                "sum residue out of range at node " +
                std::to_string(v)
            );
        }

        if (current_mode == Mode::exact_extrema &&
            node.mn > node.mx) {
            report.ok = false;
            report.violations.push_back(
                "minimum exceeds maximum at node " +
                std::to_string(v)
            );
        }

        if (current_mode == Mode::exact_extrema &&
            node.lazy.a != -1 &&
            node.lazy.a != 0 &&
            node.lazy.a != 1) {
            report.ok = false;
            report.violations.push_back(
                "invalid exact gain at node " +
                std::to_string(v)
            );
        }

        if (tl == tr) {
            if (current_mode == Mode::exact_extrema &&
                (node.mn != node.mx ||
                 node.sum != norm(node.mn))) {
                report.ok = false;
                report.violations.push_back(
                    "leaf aggregate mismatch at node " +
                    std::to_string(v)
                );
            }
            return;
        }

        int tm = tl + (tr - tl) / 2;

        self(self, 2 * v, tl, tm);
        self(self, 2 * v + 1, tm + 1, tr);

        const Node& left = tree[2 * v];
        const Node& right = tree[2 * v + 1];

        Residue child_sum = norm(
            Wide(left.sum) + right.sum
        );

        Residue expected_sum = norm(
            Wide(norm(node.lazy.a)) * child_sum +
            Wide(norm(node.lazy.b)) * (tr - tl + 1)
        );

        if (node.sum != expected_sum) {
            report.ok = false;
            report.violations.push_back(
                "parent sum/lazy mismatch at node " +
                std::to_string(v)
            );
        }

        if (current_mode == Mode::exact_extrema) {
            Value child_min = std::min(
                left.mn, right.mn
            );
            Value child_max = std::max(
                left.mx, right.mx
            );

            Wide expected_min, expected_max;

            if (node.lazy.a == 1) {
                expected_min =
                    Wide(child_min) + node.lazy.b;
                expected_max =
                    Wide(child_max) + node.lazy.b;
            }
            else if (node.lazy.a == -1) {
                expected_min =
                    -Wide(child_max) + node.lazy.b;
                expected_max =
                    -Wide(child_min) + node.lazy.b;
            }
            else {
                expected_min = node.lazy.b;
                expected_max = node.lazy.b;
            }

            if (Wide(node.mn) != expected_min ||
                Wide(node.mx) != expected_max) {
                report.ok = false;
                report.violations.push_back(
                    "parent extrema/lazy mismatch at node " +
                    std::to_string(v)
                );
            }
        }
    };

    inspect(
        inspect, 1, 0, static_cast<int>(n) - 1
    );

    return report;
}

}
