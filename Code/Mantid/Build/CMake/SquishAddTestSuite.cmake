macro( SQUISH_ADD_TEST_SUITE testName testSuite )
  message( STATUS "Creating Squish test ${testName}" )
  set( resultFile "${CMAKE_BINARY_DIR}/bin/Testing/TEST-${testName}.xml" )
  add_test(${testName}
           ${CMAKE_COMMAND}
           "-Dsquish_server_executable:STRING=${SQUISH_SERVER_EXECUTABLE}"
           "-Dsquish_client_executable:STRING=${SQUISH_CLIENT_EXECUTABLE}"
           "-Dsquish_test_suite:STRING=${testSuite}"
           "-Dsquish_results_dir:STRING=${CMAKE_BINARY_DIR}/bin/Testing"
           "-Dsquish_results_file:STRING=${resultFile}"
           "-Dmantid_cmake_modules:STRING=${MANTID_CMAKE_MODULE_PATH}"
           -P ${MANTID_CMAKE_MODULE_PATH}/SquishTestScript.cmake
          )
  set_tests_properties( ${testName} PROPERTIES FAIL_REGULAR_EXPRESSION
                        "FAILED;ERROR;FATAL"
                      )
endmacro( SQUISH_ADD_TEST_SUITE )
