function(set_pythonpath_for_cxxtests test_target_name test_files)
  # get mantid package root, which is the Framework/PythonInterface folder by
  # going up two levels, and then resolving to an absolute directory path
  get_filename_component(_pythoninterface_dir
                        "${CMAKE_SOURCE_DIR}/Framework/PythonInterface/"
                        ABSOLUTE
                        DIRECTORY)

  if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(
      _python_path
      ${PYTHON_XMLRUNNER_DIR};${_pythoninterface_dir};${PYUNITTEST_PYTHONPATH_EXTRA}$ENV{PYTHONPATH}
      )
    # cmake list separator and Windows environment separator are the same so
    # escape the cmake one
    string(REPLACE ";"
                  "\\;"
                  _python_path
                  "${_python_path}")
  else()
    string(REPLACE ";"
                  ":"
                  _python_path
                  "${PYUNITTEST_PYTHONPATH_EXTRA}")
    set(
      _python_path
      ${PYTHON_XMLRUNNER_DIR}:${_pythoninterface_dir}:${_python_path}:$ENV{PYTHONPATH}
      )
  endif()
  # Define the additional environment that will be added to every test
  list(APPEND _test_environment "PYTHONPATH=${_python_path}")
  # for each test that needs PYTHONPATH
  foreach(_the_test ${test_files})
    # make the name PythonInterfaceCppTest_testname
    set(_ctest_test_name "${test_target_name}_${_the_test}")
    # Remove the trailing .h file extension
    string(REPLACE ".h"
                   ""
                   _ctest_test_name
                   ${_ctest_test_name})
    set_tests_properties(${_ctest_test_name}
                         PROPERTIES ENVIRONMENT
                                    "${_test_environment}"
                                    TIMEOUT
                                    ${TESTING_TIMEOUT})
  endforeach(_the_test ${test_files})
endfunction()


# Top-level directory for PythonInterface tests

add_subdirectory(python)
add_subdirectory(cpp)
add_subdirectory(testhelpers)
