#include "cs253/pipeline.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

using namespace cs253;

static bool same_workload(const Workload& a, const Workload& b) {
    if (a.mode != b.mode || a.seed != b.seed || a.initial != b.initial ||
        a.operations.size() != b.operations.size()) return false;
    for (std::size_t i = 0; i < a.operations.size(); ++i) {
        const auto& x = a.operations[i];
        const auto& y = b.operations[i];
        if (x.index() != y.index()) return false;
        if (const auto* p = std::get_if<Update>(&x)) {
            const auto& q = std::get<Update>(y);
            if (p->range.begin != q.range.begin || p->range.end != q.range.end ||
                p->tag.a != q.tag.a || p->tag.b != q.tag.b) return false;
        } else {
            const auto& query_x = std::get<Query>(x);
            const auto& q = std::get<Query>(y);
            if (query_x.range.begin != q.range.begin || query_x.range.end != q.range.end) return false;
        }
    }
    return true;
}

template<typename Exception = std::exception>
static void must_fail(const std::string& text) {
    std::istringstream input(text);
    bool threw = false;
    try { (void)read_workload(input); }
    catch (const Exception&) { threw = true; }
    assert(threw);
}

static Workload round_trip(const Workload& source) {
    std::ostringstream output;
    write_workload(output, source);
    std::istringstream input(output.str());
    Workload copy = read_workload(input);
    assert(same_workload(copy, source));
    return copy;
}

int main() {
    const std::string exact =
        "CS253_V1\nEXACT\n253\n4 5\n2 4 1 7\n"
        "Q 0 4\nU 0 4 -1 3\nQ 0 4\nU 1 3 0 5\nQ 0 4\n";
    std::istringstream input(exact);
    Workload workload = round_trip(read_workload(input));
    LazySegmentTree a;
    SqrtDecomposition b;
    const ReplayResult ra = replay(a, workload);
    const ReplayResult rb = replay(b, workload);
    assert(ra.answers.size() == 3 && rb.answers.size() == 3);
    const std::int64_t sums[] = {14, modulus - 2, 7};
    const std::int64_t minima[] = {1, -4, -4};
    const std::int64_t maxima[] = {7, 2, 5};
    for (std::size_t i = 0; i < ra.answers.size(); ++i) {
        for (const Aggregate& x : {ra.answers[i], rb.answers[i]}) {
            assert(x.length == 4);
            assert(x.sum_mod == sums[i]);
            assert(x.minimum == minima[i]);
            assert(x.maximum == maxima[i]);
        }
    }
    assert(ra.elapsed_seconds >= 0 && rb.elapsed_seconds >= 0);
    assert(ra.counters.nodes_visited > 0);
    assert(rb.counters.full_blocks + rb.counters.partial_elements > 0);


    Workload empty;
    empty.seed = std::numeric_limits<Seed>::max();
    empty.operations = {Query{{0, 0}}, Update{{0, 0}, {-1, 8}}, Query{{0, 0}}};
    round_trip(empty);
    const ReplayResult er = replay(b, empty);
    assert(er.answers.size() == 2);
    for (const Aggregate& x : er.answers) {
        assert(x.length == 0 && x.sum_mod == 0);
        assert(!x.minimum && !x.maximum);
    }


    Workload modular;
    modular.mode = Mode::modular_sum;
    modular.initial = {std::numeric_limits<Value>::min(), -8};
    modular.operations = {Update{{0, 2}, {std::numeric_limits<Value>::max(), -5}}, Query{{0, 2}}};
    modular = round_trip(modular);
    const auto mod_a = replay(a, modular);
    const auto mod_b = replay(b, modular);
    assert(mod_a.answers.size() == 1 && mod_b.answers.size() == 1);
    assert(mod_a.answers[0].sum_mod == mod_b.answers[0].sum_mod);
    assert(!mod_a.answers[0].minimum && !mod_a.answers[0].maximum);


    Workload invalid = workload;
    invalid.operations.push_back(Update{{0, 4}, {2, 0}});
    bool invalid_writer = false;
    try { std::ostringstream out; write_workload(out, invalid); }
    catch (const std::invalid_argument&) { invalid_writer = true; }
    assert(invalid_writer);


    LazySegmentTree untouched;
    bool invalid_replay = false;
    try { replay(untouched, invalid); }
    catch (const std::invalid_argument&) { invalid_replay = true; }
    assert(invalid_replay);
    assert(untouched.size() == 0);

    must_fail("WRONG EXACT 1 0 0");
    must_fail("CS253_V1 BAD 1 0 0");
    must_fail("CS253_V1 EXACT -1 0 0");
    must_fail("CS253_V1 EXACT 1 -1 0");
    must_fail("CS253_V1 EXACT 1 0 -2");
    must_fail("CS253_V1 EXACT 1 0 1 X 0 0");
    must_fail("CS253_V1 EXACT 1 2 0 5");
    must_fail("CS253_V1 EXACT 1 1 1 5 Q 0");
    must_fail("CS253_V1 EXACT 1 1 1 5 Q -1 1");
    must_fail("CS253_V1 EXACT 1 1 1 5 Q 0 2");
    must_fail("CS253_V1 EXACT 1 1 1 5 Q 1 0");
    must_fail("CS253_V1 EXACT 1 1 1 5 U 0 1 2 3");
    must_fail("CS253_V1 EXACT 1 1 0 5 trailing");
    must_fail("CS253_V1 EXACT 1 1 0 9223372036854775808");
    must_fail("CS253_V1 EXACT 18446744073709551616 0 0");
    must_fail("CS253_V1 MODULAR 1 0 1 Q 0 0 more");
    must_fail("CS253_V1 EXACT 1 5000001 0");


    std::istringstream second(exact);
    assert(same_workload(read_workload(second), workload));
    std::cout << "Architect app tests passed\n";
}
