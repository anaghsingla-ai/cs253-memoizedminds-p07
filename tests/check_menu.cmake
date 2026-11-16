execute_process(
    COMMAND "${PROGRAM}" --menu
    INPUT_FILE "${INPUT_FILE}"
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Interactive menu exited ${result}: ${error}\n${output}")
endif()
foreach(expected IN ITEMS
        "Loaded 4 values"
        "A_B_ORACLE_QUERY_ANSWERS MATCH"
        "Benchmark complete"
        "mode=EXACT")
    string(FIND "${output}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Interactive menu output missing '${expected}':\n${output}")
    endif()
endforeach()
