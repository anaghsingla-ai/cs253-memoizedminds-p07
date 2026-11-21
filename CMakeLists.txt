
cmake_minimum_required(VERSION 3.16)
project(cs253_lazy_propagation LANGUAGES CXX)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cs253_contracts INTERFACE)
target_include_directories(cs253_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core data structures and Architect application.
add_library(range_structures STATIC
    src/structA/module.cpp
    src/structB/module.cpp
)
target_link_libraries(range_structures PUBLIC cs253_contracts)

add_library(app_core STATIC app/module.cpp)
target_link_libraries(app_core PUBLIC cs253_contracts)

# Oracle and Analyst implementations are kept as separately linkable modules.
add_library(oracle_core STATIC oracle/module.cpp)
target_link_libraries(oracle_core PUBLIC cs253_contracts range_structures)

add_library(bench_core STATIC bench/module.cpp)
target_link_libraries(bench_core PUBLIC cs253_contracts range_structures)
add_executable(full_scale_benchmark bench/run_full_scale.cpp)
target_link_libraries(full_scale_benchmark PRIVATE bench_core)

# Adversary generator and CLI.
add_library(cs253_adversary STATIC adversary/module.cpp)
target_link_libraries(cs253_adversary PUBLIC cs253_contracts)
add_executable(adversary_generate adversary/main.cpp)
target_link_libraries(adversary_generate PRIVATE cs253_adversary)
add_executable(adversary_replay_check adversary/replay_check.cpp)
target_link_libraries(adversary_replay_check PRIVATE cs253_adversary range_structures)

# Main calibration application.
add_executable(calibration app/main.cpp)
target_link_libraries(calibration PRIVATE app_core range_structures oracle_core bench_core cs253_adversary Threads::Threads)
target_compile_definitions(calibration PRIVATE
    CS253_SAMPLE_WORKLOAD_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_sample.txt"
)

# Tests.
add_executable(contract_smoke tests/contract_smoke.cpp)
target_link_libraries(contract_smoke PRIVATE range_structures oracle_core)

add_executable(structB_test tests/structB_test.cpp src/structB/module.cpp)
target_link_libraries(structB_test PRIVATE cs253_contracts)

add_executable(app_test tests/app_test.cpp)
target_link_libraries(app_test PRIVATE app_core range_structures)

add_executable(adversary_test tests/adversary_test.cpp)
target_link_libraries(adversary_test PRIVATE cs253_adversary range_structures)

add_executable(oracle_test tests/oracle_test.cpp)
target_link_libraries(oracle_test PRIVATE oracle_core range_structures)

add_executable(differential_harness tests/differential_test.cpp)
target_link_libraries(differential_harness PRIVATE oracle_core range_structures)

add_executable(benchmark_smoke tests/benchmark_smoke.cpp)
target_link_libraries(benchmark_smoke PRIVATE bench_core app_core range_structures)

enable_testing()
add_test(NAME contract_smoke COMMAND contract_smoke)
add_test(NAME structB_test COMMAND structB_test)
add_test(NAME app_test COMMAND app_test)
add_test(NAME adversary_test COMMAND adversary_test)
foreach(target IN ITEMS push_downs alternating_negative_gain block_friendly range_width_sweep)
    foreach(mode IN ITEMS EXACT MODULAR)
        add_test(NAME replay_${target}_${mode}
            COMMAND adversary_replay_check ${target} ${mode} 4097 8192 253 63)
    endforeach()
endforeach()
add_test(NAME adversary_scale COMMAND adversary_test --scale)
set_tests_properties(adversary_scale PROPERTIES LABELS scale TIMEOUT 180)
add_test(NAME adversary_cli COMMAND adversary_generate alternating_negative_gain 17 64 253 3 EXACT)
add_test(NAME adversary_cli_invalid COMMAND adversary_generate push_downs 17 64 253 0 EXACT)
set_tests_properties(adversary_cli_invalid PROPERTIES WILL_FAIL TRUE)
add_test(NAME oracle_test COMMAND oracle_test)
add_test(NAME differential_harness COMMAND differential_harness)
set_tests_properties(differential_harness PROPERTIES TIMEOUT 300)
add_test(NAME benchmark_smoke COMMAND benchmark_smoke)
add_test(NAME calibration_cli
    COMMAND ${CMAKE_COMMAND}
        -DPROGRAM=$<TARGET_FILE:calibration>
        -DWORKLOAD=${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_sample.txt
        -DMODULAR_WORKLOAD=${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_modular_sample.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_cli.cmake
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/menu_smoke_input.txt"
    "1\n${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_sample.txt\n4\n4\n6\n${CMAKE_CURRENT_BINARY_DIR}/menu_smoke.csv\n7\n0\n")
add_test(NAME calibration_menu
    COMMAND ${CMAKE_COMMAND}
        -DPROGRAM=$<TARGET_FILE:calibration>
        -DINPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/menu_smoke_input.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_menu.cmake
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target IN ITEMS range_structures app_core oracle_core bench_core cs253_adversary
                            full_scale_benchmark adversary_generate adversary_replay_check calibration contract_smoke structB_test app_test
                            adversary_test oracle_test differential_harness benchmark_smoke)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
    foreach(target IN ITEMS contract_smoke structB_test app_test adversary_test oracle_test
                            differential_harness benchmark_smoke)
        target_compile_options(${target} PRIVATE -UNDEBUG)
    endforeach()
endif()

# Run the complete automated suite; calibration --all also exercises the
# sample A/B/Oracle comparison and the integrated benchmark runner.
add_custom_target(all_checks
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS calibration contract_smoke structB_test app_test adversary_test
            oracle_test differential_harness benchmark_smoke adversary_generate adversary_replay_check
    USES_TERMINAL
)

