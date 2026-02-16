cmake_minimum_required(VERSION 3.28)

# Usage: cmake -DBINARY_DIR=<dir> -DTARGET=<target> -DEXPECTED_MESSAGE=<msg> -P compile_fail_test.cmake

if(NOT DEFINED BINARY_DIR
   OR NOT DEFINED TARGET
   OR NOT DEFINED EXPECTED_MESSAGE)
    message(FATAL_ERROR "BINARY_DIR, TARGET and EXPECTED_MESSAGE must be defined")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${BINARY_DIR} --target ${TARGET}
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE build_output
    RESULT_VARIABLE build_result)

if(build_result EQUAL 0)
    message(FATAL_ERROR "Build succeeded but should have failed")
endif()

string(FIND "${build_output}" "${EXPECTED_MESSAGE}" match_pos)
if(match_pos EQUAL -1)
    message(FATAL_ERROR "Build failed but expected static_assert message not found.\n"
                        "Expected: ${EXPECTED_MESSAGE}\n" "Got:\n${build_output}")
endif()
