cmake_minimum_required(VERSION 3.16)
project(cs253_lazy_propagation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cs253_contracts INTERFACE)
target_include_directories(cs253_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)


add_library(range_structures STATIC
  src/structA/module.cpp
  src/structB/module.cpp
)
target_link_libraries(range_structures PUBLIC cs253_contracts)


add_library(app_core STATIC app/module.cpp)
target_link_libraries(app_core PUBLIC cs253_contracts)



add_library(support_modules OBJECT
  oracle/module.cpp
  bench/module.cpp
)
target_link_libraries(support_modules PRIVATE cs253_contracts)

# Part E: completed adversarial workload generator and its standalone CLI.
add_library(cs253_adversary STATIC adversary/module.cpp)
target_link_libraries(cs253_adversary PUBLIC cs253_contracts)
add_executable(adversary_generate adversary/main.cpp)
target_link_libraries(adversary_generate PRIVATE cs253_adversary)
add_executable(adversary_test tests/adversary_test.cpp src/structA/module.cpp src/structB/module.cpp)
target_link_libraries(adversary_test PRIVATE cs253_adversary cs253_contracts)

add_executable(calibration app/main.cpp)
target_link_libraries(calibration PRIVATE app_core range_structures)
target_compile_definitions(calibration PRIVATE
  CS253_SAMPLE_WORKLOAD_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_sample.txt"
)

add_executable(contract_smoke tests/contract_smoke.cpp)
target_link_libraries(contract_smoke PRIVATE range_structures)

add_executable(structB_test tests/structB_test.cpp src/structB/module.cpp)
target_link_libraries(structB_test PRIVATE cs253_contracts)

add_executable(app_test tests/app_test.cpp)
target_link_libraries(app_test PRIVATE app_core range_structures)

enable_testing()
add_test(NAME contract_smoke COMMAND contract_smoke)
add_test(NAME structB_test COMMAND structB_test)
add_test(NAME app_test COMMAND app_test)
add_test(NAME adversary_test COMMAND adversary_test)
add_test(NAME adversary_scale COMMAND adversary_test --scale)
set_tests_properties(adversary_scale PROPERTIES LABELS scale TIMEOUT 180)
add_test(NAME adversary_cli COMMAND adversary_generate alternating_negative_gain 17 64 253 3 EXACT)
add_test(NAME adversary_cli_invalid COMMAND adversary_generate push_downs 17 64 253 0 EXACT)
set_tests_properties(adversary_cli_invalid PROPERTIES WILL_FAIL TRUE)
add_test(NAME calibration_cli
  COMMAND ${CMAKE_COMMAND}
    -DPROGRAM=$<TARGET_FILE:calibration>
    -DWORKLOAD=${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_sample.txt
    -DMODULAR_WORKLOAD=${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_modular_sample.txt
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_cli.cmake
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(target IN ITEMS range_structures app_core support_modules cs253_adversary
                          adversary_generate adversary_test contract_smoke calibration structB_test app_test)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endforeach()

  target_compile_options(contract_smoke PRIVATE -UNDEBUG)
  target_compile_options(structB_test PRIVATE -UNDEBUG)
  target_compile_options(app_test PRIVATE -UNDEBUG)
  target_compile_options(adversary_test PRIVATE -UNDEBUG)
endif()



add_custom_target(all_checks
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  COMMAND $<TARGET_FILE:calibration> --all
  DEPENDS calibration contract_smoke structB_test app_test support_modules
          adversary_generate adversary_test
  USES_TERMINAL
)
