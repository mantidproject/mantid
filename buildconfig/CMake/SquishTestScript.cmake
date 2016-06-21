#
# This script launches a GUI test suite using Squish.  You should not call
# the script directly; instead, you should access it via the
# SQUISH_ADD_TEST_SUITE macro.
#
# This script starts the Squish server, launches the test suite on the
# client, and finally stops the squish server.  If any of these steps
# fail (including if the tests do not pass) then a fatal error is
# raised.
#
# Based on the SQUISH_ADD_TEST macro
#
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

message(STATUS "squish_server_executable='${squish_server_executable}'")
message(STATUS "squish_aut='${squish_aut}'")
message(STATUS "squish_aut_path='${squish_aut_path}'")
message(STATUS "squish_client_executable='${squish_client_executable}'")
message(STATUS "squish_test_suite='${squish_test_suite}'")
message(STATUS "squish_env_vars=${squish_env_vars}")
message(STATUS "squish_results_dir='${squish_results_dir}'")
message(STATUS "squish_results_file='${squish_results_file}'")

# parse environment variables
foreach(i ${squish_env_vars})
  message(STATUS "parsing env var key/value pair ${i}")
  string(REGEX MATCH "([^=]*)=(.*)" squish_env_name ${i})
  message(STATUS "key=${CMAKE_MATCH_1}")
  message(STATUS "value=${CMAKE_MATCH_2}")
  set(ENV{${CMAKE_MATCH_1}} ${CMAKE_MATCH_2})
endforeach()

# run the test
if (WIN32)
  execute_process(
    COMMAND ${mantid_cmake_modules}/SquishRunTestSuite.bat ${squish_server_executable} ${squish_aut} ${squish_aut_path} ${squish_client_executable} ${squish_test_suite} ${squish_results_dir} ${squish_results_file}
    RESULT_VARIABLE test_rv
    )
endif (WIN32)

if (UNIX)
  execute_process(
    COMMAND  ${mantid_cmake_modules}/SquishRunTestSuite.sh ${squish_server_executable} ${squish_aut} ${squish_aut_path} ${squish_client_executable} ${squish_test_suite} ${squish_results_dir} ${squish_results_file}
    RESULT_VARIABLE test_rv
    )
endif (UNIX)

# check for an error with running the test
if(NOT "${test_rv}" STREQUAL "0")
  message(FATAL_ERROR "Error running Squish test")
endif(NOT "${test_rv}" STREQUAL "0")

file(READ ${squish_results_file} error_log)
message(STATUS ${error_log})

