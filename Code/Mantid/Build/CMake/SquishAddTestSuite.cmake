##############################################################################
#
# This macro will add a list of test suites into the ctest mechanism. The
# ctest name is generated from the name of the
#
##############################################################################
macro( SQUISH_ADD_TEST_SUITE )
  foreach( _test_suite_path ${ARGN} )
    set( testSuite ${CMAKE_CURRENT_SOURCE_DIR}/${_test_suite_path} )
    string( REGEX MATCH "/.+$" _test_suite ${_test_suite_path} )
    string( REGEX REPLACE "/" "" _test_suite ${_test_suite} )
    string( REGEX REPLACE "suite_" "" _test_suite ${_test_suite} )
    set( testName ${_test_suite}SquishTests )
    #message( STATUS "Creating Squish test ${testName}" )
    set( resultFile "${CMAKE_BINARY_DIR}/bin/Testing/TEST-${testName}.xml" )
    add_test(${testName}
             ${CMAKE_COMMAND}
             "-Dsquish_server_executable:STRING=${SQUISH_SERVER_EXECUTABLE}"
             "-Dsquish_client_executable:STRING=${SQUISH_CLIENT_EXECUTABLE}"
             "-Dsquish_aut:STRING=${SQUISH_AUT}"
             "-Dsquish_aut_path:STRING=${SQUISH_AUT_PATH}"
             "-Dsquish_test_suite:STRING=${testSuite}"
             "-Dsquish_env_vars:STRING=${SQUISH_ENV_VARS}"
             "-Dsquish_results_dir:STRING=${CMAKE_BINARY_DIR}/bin/Testing"
             "-Dsquish_results_file:STRING=${resultFile}"
             "-Dmantid_cmake_modules:STRING=${MANTID_CMAKE_MODULE_PATH}"
             -P ${MANTID_CMAKE_MODULE_PATH}/SquishTestScript.cmake
            )
    set_tests_properties( ${testName} PROPERTIES FAIL_REGULAR_EXPRESSION
                          "FAILURE;FAILED;ERROR;FATAL"
                        )
  endforeach( )
endmacro( SQUISH_ADD_TEST_SUITE )

