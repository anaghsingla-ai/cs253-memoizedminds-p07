#pragma once

#include "cs253/types.hpp"
#include <ostream>

namespace cs253 {
namespace adversary {

inline void export_workload(std::ostream& output, const Workload& workload) {
    output << "CS253_V1\n";
    if (workload.mode == Mode::exact_extrema) {
        output << "EXACT\n";
    } else {
        output << "MODULAR\n";
    }

    output << workload.seed << '\n';
    output << workload.initial.size() << ' ' << workload.operations.size() << '\n';

    for (std::size_t i = 0; i < workload.initial.size(); i++) {
        output << workload.initial[i] << ' ';
    }
    output << '\n';

    for (std::size_t i = 0; i < workload.operations.size(); i++) {
        const Operation& operation = workload.operations[i];
        if (std::holds_alternative<Update>(operation)) {
            const Update& update = std::get<Update>(operation);
            output << "U " << update.range.begin << ' ' << update.range.end;
            output << ' ' << update.tag.a << ' ' << update.tag.b << '\n';
        } else {
            const Query& query = std::get<Query>(operation);
            output << "Q " << query.range.begin << ' ' << query.range.end << '\n';
        }
    }
}

}
}
