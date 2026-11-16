
cmake_minimum_required(VERSION 3.16)
project(cs253_lazy_propagation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cs253_contracts INTERFACE)
target_include_directories(cs253_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(module_placeholders OBJECT
    src/structA/module.cpp
    src/structB/module.cpp
    app/module.cpp
    oracle/module.cpp
    adversary/module.cpp
    bench/module.cpp
)
target_link_libraries(module_placeholders PRIVATE cs253_contracts)

add_executable(calibration app/main.cpp)
target_link_libraries(calibration PRIVATE cs253_contracts)

add_executable(contract_smoke tests/contract_smoke.cpp)
target_link_libraries(contract_smoke PRIVATE cs253_contracts)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(module_placeholders PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(contract_smoke PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_test(NAME contract_smoke COMMAND contract_smoke)

add_custom_target(all_checks
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND $<TARGET_FILE:calibration> --all
    DEPENDS contract_smoke calibration module_placeholders
    USES_TERMINAL
)

add_test(NAME scaffold_status COMMAND calibration --status)

add_test(
    NAME unfinished_pipeline
    COMMAND ${CMAKE_COMMAND}
        -DPROGRAM=$<TARGET_FILE:calibration>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_unfinished.cmake
)

# Structure B algorithm tests
add_executable(structB_test
    tests/structB_test.cpp
    src/structB/module.cpp
)
target_link_libraries(structB_test PRIVATE cs253_contracts)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(structB_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME structB_test COMMAND structB_test)


# Member 5: generators preserve pipeline.hpp and do not depend on unfinished owners.
# Member 5: Adversarial generators
add_library(cs253_adversary STATIC adversary/module.cpp)
target_link_libraries(cs253_adversary PUBLIC cs253_contracts)

add_executable(adversary_generate adversary/main.cpp)
target_link_libraries(adversary_generate PRIVATE cs253_adversary)

add_executable(adversary_test
    tests/adversary_test.cpp
    src/structA/module.cpp
    src/structB/module.cpp
)
target_link_libraries(adversary_test PRIVATE cs253_adversary)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cs253_adversary PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(adversary_generate PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(adversary_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME adversary_test COMMAND adversary_test)
add_test(NAME adversary_scale COMMAND adversary_test --scale)

set_tests_properties(adversary_scale PROPERTIES
    LABELS scale
    TIMEOUT 180
)

add_test(
    NAME adversary_cli
    COMMAND adversary_generate alternating_negative_gain 17 64 253 3 EXACT
)

add_test(
    NAME adversary_cli_invalid
    COMMAND adversary_generate push_downs 17 64 253 0 EXACT
)

set_tests_properties(adversary_cli_invalid PROPERTIES WILL_FAIL TRUE)

add_dependencies(all_checks structB_test adversary_test adversary_generate)

# Array Oracle and differential testing
add_executable(oracle_test
    tests/oracle_test.cpp
    oracle/module.cpp
    src/structA/module.cpp
    src/structB/module.cpp
)

target_link_libraries(oracle_test PRIVATE cs253_contracts)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(oracle_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME oracle_test COMMAND oracle_test)

# Oracle owner: differential test harness (D4)
# Links oracle, structA, and structB — no other owner's modules needed.
add_executable(differential_harness
    tests/differential_test.cpp
    oracle/module.cpp
    src/structA/module.cpp
    src/structB/module.cpp
)
target_link_libraries(differential_harness PRIVATE cs253_contracts)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(differential_harness PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Full 1M-op run (both modes + edge cases + shrink).
add_test(NAME differential_harness COMMAND differential_harness)
set_tests_properties(differential_harness PROPERTIES TIMEOUT 300)

add_dependencies(all_checks oracle_test differential_harness)
