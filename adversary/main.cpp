#include "cs253/pipeline.hpp"
#include "export.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
std::uint64_t number(const char* arg) {
    const std::string s(arg);
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("expected an unsigned decimal integer");
    std::size_t end;
    const auto v = std::stoull(s, &end);
    if (end != s.size()) throw std::invalid_argument("invalid number");
    return v;
}
std::size_t size_arg(const char* arg) {
    const auto v = number(arg);
    if (v > std::numeric_limits<std::size_t>::max()) throw std::out_of_range("size too large");
    return static_cast<std::size_t>(v);
}
void help() {
    std::cout << "adversary_generate TARGET N Q SEED WIDTH EXACT|MODULAR [OUTPUT]\n"
                 "Targets: push_downs alternating_negative_gain block_friendly range_width_sweep\n"
                 "Writes CS253_V1 to OUTPUT or stdout. WIDTH must be in [1,N] for N>0.\n";
}
}
int main(int argc, char** argv) {
    using namespace cs253;
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") { help(); return 0; }
        if (argc != 7 && argc != 8) { help(); return 2; }
        const std::string name = argv[1];
        AdversarialTarget target;
        if (name == "push_downs") target = AdversarialTarget::push_downs;
        else if (name == "alternating_negative_gain") target = AdversarialTarget::alternating_negative_gain;
        else if (name == "block_friendly") target = AdversarialTarget::block_friendly;
        else if (name == "range_width_sweep") target = AdversarialTarget::range_width_sweep;
        else throw std::invalid_argument("unknown target");
        GeneratorConfig c;
        c.elements = size_arg(argv[2]); c.operations = size_arg(argv[3]);
        c.seed = number(argv[4]); c.range_width = size_arg(argv[5]);
        const std::string mode = argv[6];
        if (mode == "EXACT") c.mode = Mode::exact_extrema;
        else if (mode == "MODULAR") c.mode = Mode::modular_sum;
        else throw std::invalid_argument("mode must be EXACT or MODULAR");
        const auto w = generate(target, c);
        if (argc == 8) {
            std::ofstream out(argv[7]);
            if (!out) throw std::runtime_error("cannot open output file");
            adversary::export_workload(out, w);
            out.close();
            if (!out) throw std::runtime_error("output write failed");
        } else {
            adversary::export_workload(std::cout, w);
            if (!std::cout) throw std::runtime_error("output write failed");
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "adversary_generate: " << e.what() << '\n';
        return 2;
    }
}
