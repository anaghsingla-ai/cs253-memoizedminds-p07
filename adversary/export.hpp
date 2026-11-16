#pragma once
#include "cs253/types.hpp"
#include <ostream>
namespace cs253::adversary {
// Module-local exporter until the Architect provides write_workload().
// Deliberately does not define or replace that shared function.
inline void export_workload(std::ostream& out, const Workload& w) {
    out << "CS253_V1\n" << (w.mode == Mode::exact_extrema ? "EXACT" : "MODULAR")
        << '\n' << w.seed << '\n' << w.initial.size() << ' ' << w.operations.size() << '\n';
    for (auto x : w.initial) out << x << ' ';
    out << '\n';
    for (const auto& op : w.operations) {
        if (const auto* u = std::get_if<Update>(&op))
            out << "U " << u->range.begin << ' ' << u->range.end << ' ' << u->tag.a << ' ' << u->tag.b << '\n';
        else {
            const auto r = std::get<Query>(op).range;
            out << "Q " << r.begin << ' ' << r.end << '\n';
        }
    }
}
} // namespace cs253::adversary
