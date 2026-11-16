#include "cs253/pipeline.hpp"
#include <chrono>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace cs253 {

namespace {

Workload set_workload_width(const Workload& base_work, std::size_t target_width) {
    std::size_t n = base_work.initial.size();
    if (target_width == 0 || n == 0) {
        return base_work;
    }

    std::size_t w = target_width;
    if (w > n) {
        w = n;
    }

    Workload w_work = base_work;
    std::size_t max_pos = n - w;

    for (std::size_t i = 0; i < w_work.operations.size(); ++i) {
        if (std::holds_alternative<Update>(w_work.operations[i])) {
            Update u = std::get<Update>(w_work.operations[i]);
            if (u.range.end - u.range.begin != w) {
                std::size_t l = (max_pos == 0) ? 0 : (u.range.begin % (max_pos + 1));
                u.range.begin = l;
                u.range.end = l + w;
            }
            w_work.operations[i] = u;
        } else {
            Query q = std::get<Query>(w_work.operations[i]);
            if (q.range.end - q.range.begin != w) {
                std::size_t l = (max_pos == 0) ? 0 : (q.range.begin % (max_pos + 1));
                q.range.begin = l;
                q.range.end = l + w;
            }
            w_work.operations[i] = q;
        }
    }

    return w_work;
}

void check_answers_match(const Factory& make_a, const Factory& make_b, const Workload& work) {
    auto a = make_a();
    auto b = make_b();
    if (!a || !b) {
        throw std::invalid_argument("null factory given to benchmark");
    }

    a->build(work.initial, work.mode);
    b->build(work.initial, work.mode);

    for (std::size_t i = 0; i < work.operations.size(); ++i) {
        if (std::holds_alternative<Update>(work.operations[i])) {
            Update u = std::get<Update>(work.operations[i]);
            a->update(u.range, u.tag);
            b->update(u.range, u.tag);
        } else {
            Query q = std::get<Query>(work.operations[i]);
            Aggregate x = a->query(q.range);
            Aggregate y = b->query(q.range);

            if (x.length != y.length ||
                x.sum_mod != y.sum_mod ||
                x.minimum != y.minimum ||
                x.maximum != y.maximum) {
                throw std::logic_error("structures A and B did not match during validation");
            }
        }
    }
}

Measurement run_single_trial(const Factory& make_struct,
                             const std::string& struct_name,
                             const Workload& work,
                             Seed seed_val,
                             std::size_t width_val,
                             std::size_t rep_num) {
    auto s = make_struct();
    s->build(work.initial, work.mode);
    s->reset_counters();

    Residue check_sum = 0;
    auto t1 = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < work.operations.size(); ++i) {
        if (std::holds_alternative<Update>(work.operations[i])) {
            const Update& u = std::get<Update>(work.operations[i]);
            s->update(u.range, u.tag);
        } else {
            const Query& q = std::get<Query>(work.operations[i]);
            Aggregate a = s->query(q.range);
            check_sum ^= a.sum_mod + static_cast<Residue>(a.length);
            if (a.minimum.has_value()) {
                check_sum ^= *a.minimum;
            }
            if (a.maximum.has_value()) {
                check_sum ^= *a.maximum;
            }
        }
    }

    auto t2 = std::chrono::steady_clock::now();
    std::chrono::duration<double> d = t2 - t1;

    Measurement m;
    m.structure = struct_name;
    m.workload = (work.mode == Mode::exact_extrema) ? "exact_extrema" : "modular_sum";
    m.seed = seed_val;
    m.n = work.initial.size();
    m.operations = work.operations.size();
    m.width = width_val;
    m.repetition = rep_num;
    m.elapsed_seconds = d.count();
    m.counters = s->counters();

    if (check_sum == -999999999) {
        m.elapsed_seconds += 0.0;
    }

    return m;
}

}

std::vector<Measurement> benchmark(const Factory& a, const Factory& b,
                                   const Workload& workload,
                                   const BenchmarkConfig& config) {
    if (!a || !b) {
        throw std::invalid_argument("benchmark factories cannot be null");
    }

    std::size_t total_reps = config.repetitions;
    if (total_reps < 5) {
        total_reps = 5;
    }

    std::vector<std::size_t> widths = config.widths;
    if (widths.empty()) {
        widths.push_back(0);
    }

    Seed used_seed = (workload.seed != 0) ? workload.seed : config.seed;
    std::vector<Measurement> all_rows;

    for (std::size_t i = 0; i < widths.size(); ++i) {
        std::size_t w = widths[i];
        Workload cur_work = set_workload_width(workload, w);

        std::size_t real_w = w;
        if (real_w == 0 && !cur_work.operations.empty()) {
            if (std::holds_alternative<Update>(cur_work.operations[0])) {
                Range r = std::get<Update>(cur_work.operations[0]).range;
                real_w = r.end - r.begin;
            } else {
                Range r = std::get<Query>(cur_work.operations[0]).range;
                real_w = r.end - r.begin;
            }
        }

        check_answers_match(a, b, cur_work);

        for (std::size_t r = 1; r <= total_reps; ++r) {
            Measurement ma = run_single_trial(a, "structA", cur_work, used_seed, real_w, r);
            all_rows.push_back(ma);

            Measurement mb = run_single_trial(b, "structB", cur_work, used_seed, real_w, r);
            all_rows.push_back(mb);
        }
    }

    return all_rows;
}

void write_csv(std::ostream& output, const std::vector<Measurement>& rows) {
    output << "structure,workload,seed,n,operations,width,repetition,"
           << "elapsed_seconds,nodes_visited,push_downs,full_blocks,"
           << "partial_elements,tag_compositions,aggregate_merges\n";

    std::ios_base::fmtflags f = output.flags();
    std::streamsize p = output.precision();
    output << std::fixed << std::setprecision(9);

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const Measurement& m = rows[i];
        output << m.structure << ","
               << m.workload << ","
               << m.seed << ","
               << m.n << ","
               << m.operations << ","
               << m.width << ","
               << m.repetition << ","
               << m.elapsed_seconds << ","
               << m.counters.nodes_visited << ","
               << m.counters.push_downs << ","
               << m.counters.full_blocks << ","
               << m.counters.partial_elements << ","
               << m.counters.tag_compositions << ","
               << m.counters.aggregate_merges << "\n";
    }

    output.flags(f);
    output.precision(p);
}

}
