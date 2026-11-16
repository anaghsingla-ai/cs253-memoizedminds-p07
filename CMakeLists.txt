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



add_library(module_placeholders OBJECT
  oracle/module.cpp
  adversary/module.cpp
  bench/module.cpp
)
target_link_libraries(module_placeholders PRIVATE cs253_contracts)

add_executable(calibration app/main.cpp)
target_link_libraries(calibration PRIVATE app_core range_structures)
target_compile_definitions(calibration PRIVATE
  CS253_SAMPLE_WORKLOAD_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_sample.txt"
)

add_executable(contract_smoke tests/contract_smoke.cpp)
target_link_libraries(contract_smoke PRIVATE cs253_contracts)

add_executable(structB_test tests/structB_test.cpp src/structB/module.cpp)
target_link_libraries(structB_test PRIVATE cs253_contracts)

add_executable(app_test tests/app_test.cpp)
target_link_libraries(app_test PRIVATE app_core range_structures)

enable_testing()
add_test(NAME contract_smoke COMMAND contract_smoke)
add_test(NAME structB_test COMMAND structB_test)
add_test(NAME app_test COMMAND app_test)
add_test(NAME calibration_cli
  COMMAND ${CMAKE_COMMAND}
    -DPROGRAM=$<TARGET_FILE:calibration>
    -DWORKLOAD=${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_sample.txt
    -DMODULAR_WORKLOAD=${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/architect_modular_sample.txt
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_cli.cmake
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(target IN ITEMS range_structures app_core module_placeholders contract_smoke
                          calibration structB_test app_test)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endforeach()

  target_compile_options(contract_smoke PRIVATE -UNDEBUG)
  target_compile_options(structB_test PRIVATE -UNDEBUG)
  target_compile_options(app_test PRIVATE -UNDEBUG)
endif()



add_custom_target(all_checks
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  COMMAND $<TARGET_FILE:calibration> --all
  DEPENDS calibration contract_smoke structB_test app_test module_placeholders
  USES_TERMINAL
)
