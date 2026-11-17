// Generator command line. Tool assistance is declared in TOOLS.md.
#include "cs253/pipeline.hpp"
#include "export.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

std::uint64_t number(const char* argument) {
    std::string text = argument;
    if (text.empty()) {
        throw std::invalid_argument("expected an unsigned decimal integer");
    }
    for (std::size_t i = 0; i < text.size(); i++) {
        if (text[i] < '0' || text[i] > '9') {
            throw std::invalid_argument("expected an unsigned decimal integer");
        }
    }

    std::size_t characters_read = 0;
    unsigned long long value = std::stoull(text, &characters_read);
    if (characters_read != text.size()) {
        throw std::invalid_argument("invalid number");
    }
    if (value > std::numeric_limits<std::uint64_t>::max()) {
        throw std::out_of_range("number too large");
    }
    return static_cast<std::uint64_t>(value);
}

std::size_t size_arg(const char* argument) {
    std::uint64_t value = number(argument);
    if (value > std::numeric_limits<std::size_t>::max()) {
        throw std::out_of_range("size too large");
    }
    return static_cast<std::size_t>(value);
}

void help() {
    std::cout << "adversary_generate TARGET N Q SEED WIDTH EXACT|MODULAR [OUTPUT]\n";
    std::cout << "Targets: push_downs alternating_negative_gain block_friendly range_width_sweep\n";
    std::cout << "Writes CS253_V1 to OUTPUT or stdout. WIDTH must be in [1,N] for N>0.\n";
}

} // namespace

int main(int argc, char** argv) {
    using namespace cs253;

    try {
        if (argc == 2) {
            if (std::string(argv[1]) == "--help") {
                help();
                return 0;
            }
        }
        if (argc != 7 && argc != 8) {
            help();
            return 2;
        }

        std::string target_name = argv[1];
        AdversarialTarget target;
        if (target_name == "push_downs") {
            target = AdversarialTarget::push_downs;
        } else if (target_name == "alternating_negative_gain") {
            target = AdversarialTarget::alternating_negative_gain;
        } else if (target_name == "block_friendly") {
            target = AdversarialTarget::block_friendly;
        } else if (target_name == "range_width_sweep") {
            target = AdversarialTarget::range_width_sweep;
        } else {
            throw std::invalid_argument("unknown target");
        }

        GeneratorConfig config;
        config.elements = size_arg(argv[2]);
        config.operations = size_arg(argv[3]);
        config.seed = number(argv[4]);
        config.range_width = size_arg(argv[5]);

        std::string mode_name = argv[6];
        if (mode_name == "EXACT") {
            config.mode = Mode::exact_extrema;
        } else if (mode_name == "MODULAR") {
            config.mode = Mode::modular_sum;
        } else {
            throw std::invalid_argument("mode must be EXACT or MODULAR");
        }

        Workload workload = generate(target, config);
        if (argc == 8) {
            std::ofstream output(argv[7]);
            if (!output) {
                throw std::runtime_error("cannot open output file");
            }
            adversary::export_workload(output, workload);
            output.close();
            if (!output) {
                throw std::runtime_error("output write failed");
            }
        } else {
            adversary::export_workload(std::cout, workload);
            if (!std::cout) {
                throw std::runtime_error("output write failed");
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "adversary_generate: " << error.what() << '\n';
        return 2;
    }
}
