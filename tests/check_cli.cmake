if(NOT DEFINED PROGRAM OR NOT DEFINED WORKLOAD)
  message(FATAL_ERROR "PROGRAM and WORKLOAD are required")
endif()

execute_process(COMMAND "${PROGRAM}" --help
  RESULT_VARIABLE help_status OUTPUT_VARIABLE help_out ERROR_VARIABLE help_err)
if(NOT help_status EQUAL 0 OR NOT help_out MATCHES "calibration --run")
  message(FATAL_ERROR "--help failed: ${help_err}${help_out}")
endif()

execute_process(COMMAND "${PROGRAM}" --status
  RESULT_VARIABLE status_code OUTPUT_VARIABLE status_out ERROR_VARIABLE status_err)
if(NOT status_code EQUAL 0 OR NOT status_out MATCHES "parser=ready" OR
   NOT status_out MATCHES "full_pipeline=blocked")
  message(FATAL_ERROR "--status is inaccurate: ${status_err}${status_out}")
endif()

foreach(structure IN ITEMS A B both)
  execute_process(COMMAND "${PROGRAM}" --run "${WORKLOAD}" --structure "${structure}"
    RESULT_VARIABLE run_code OUTPUT_VARIABLE run_out ERROR_VARIABLE run_err)
  if(NOT run_code EQUAL 0)
    message(FATAL_ERROR "--run ${structure} failed: ${run_err}${run_out}")
  endif()
  if(NOT run_out MATCHES "Q 0 LEN 4 SUM 14 MIN 1 MAX 7")
    message(FATAL_ERROR "--run ${structure} returned unexpected query output: ${run_out}")
  endif()
  if(NOT run_out MATCHES "ELAPSED_SECONDS" OR NOT run_out MATCHES "OPS_PER_SECOND")
    message(FATAL_ERROR "--run ${structure} omitted timing/throughput: ${run_out}")
  endif()
  if(structure STREQUAL "both" AND NOT run_out MATCHES "A_B_QUERY_ANSWERS MATCH")
    message(FATAL_ERROR "--structure both did not report the A/B comparison: ${run_out}")
  endif()
endforeach()

execute_process(COMMAND "${PROGRAM}" --run "${WORKLOAD}" --structure oracle
  RESULT_VARIABLE oracle_code OUTPUT_VARIABLE oracle_out ERROR_VARIABLE oracle_err)
if(NOT oracle_code EQUAL 0 OR NOT oracle_out MATCHES "STRUCTURE ORACLE")
  message(FATAL_ERROR "Oracle selector failed: ${oracle_err}${oracle_out}")
endif()

if(DEFINED MODULAR_WORKLOAD)
  execute_process(COMMAND "${PROGRAM}" --run "${MODULAR_WORKLOAD}" --structure both
    RESULT_VARIABLE modular_code OUTPUT_VARIABLE modular_out ERROR_VARIABLE modular_err)
  if(NOT modular_code EQUAL 0 OR
     NOT modular_out MATCHES "Q 0 LEN 3 SUM 10 MIN NA MAX NA" OR
     NOT modular_out MATCHES "Q 1 LEN 3 SUM 6 MIN NA MAX NA" OR
     NOT modular_out MATCHES "Q 2 LEN 0 SUM 0 MIN NA MAX NA")
    message(FATAL_ERROR "MODULAR CLI replay failed: ${modular_err}${modular_out}")
  endif()
endif()

execute_process(COMMAND "${PROGRAM}" --run "${WORKLOAD}" --structure not-a-structure
  RESULT_VARIABLE invalid_structure_code OUTPUT_VARIABLE invalid_structure_out
  ERROR_VARIABLE invalid_structure_err)
if(invalid_structure_code EQUAL 0 OR NOT invalid_structure_err MATCHES "Unknown structure")
  message(FATAL_ERROR "Unknown structure must fail explicitly: ${invalid_structure_err}${invalid_structure_out}")
endif()

execute_process(COMMAND "${PROGRAM}" --run "${WORKLOAD}.missing" --structure A
  RESULT_VARIABLE missing_file_code OUTPUT_VARIABLE missing_file_out ERROR_VARIABLE missing_file_err)
if(missing_file_code EQUAL 0 OR NOT missing_file_err MATCHES "Cannot open workload file")
  message(FATAL_ERROR "Missing workload must fail explicitly: ${missing_file_err}${missing_file_out}")
endif()

execute_process(COMMAND "${PROGRAM}" --all
  RESULT_VARIABLE all_code OUTPUT_VARIABLE all_out ERROR_VARIABLE all_err)
if(NOT all_code EQUAL 3 OR NOT all_out MATCHES "AVAILABLE_APP_SMOKE PASS" OR
   NOT all_out MATCHES "BLOCKED: benchmark runner")
  message(FATAL_ERROR "--all must pass available smoke checks and report blockers: ${all_err}${all_out}")
endif()

message(STATUS "CLI integration checks passed; full --all is correctly marked blocked")
