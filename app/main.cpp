#include <iostream>
#include <string>
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--status") {
        std::cout << "CS253 Project 7 scaffold: interfaces ready; six modules pending.\n";
        return 0;
    }
    std::cerr << "NOT IMPLEMENTED: calibration replay, differential tests and benchmarks.\n"
              << "Use --status for scaffold status. --all is reserved for the final pipeline.\n";
    return 2;
}
