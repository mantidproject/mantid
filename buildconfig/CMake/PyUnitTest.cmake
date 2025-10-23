# PYUNITTEST_ADD_TEST (public macro to add unit tests) Adds a set of python tests based upon the unittest module
#
# The variable PYUNITTEST_PYTHONPATH_EXTRA can be defined with extra paths to add to PYTHONPATH during the tests
# Parameters: _test_src_dir :: A base directory when added to the relative test paths gives an absolute path to that
# test. This directory is added to the PYTHONPATH when tests are executed _testname_prefix :: A prefix for each test
# that is added to ctest, the name will be ${_testname_prefix}_TestName ${ARGN} :: List of test files
function(PYUNITTEST_ADD_TEST _test_src_dir _testname_prefix)
  if(NOT PYUNITTEST_RUNNER)
    set(_test_runner_module ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/test/testhelpers/testrunner.py)
  else()
    set(_test_runner_module ${PYUNITTEST_RUNNER})
  endif()

  py_add_test("UnitTest" ${_test_runner_module} ${ARGV})

endfunction()

# PYSYSTEMTEST_ADD_TEST (public macro to add system tests) Adds a set of python tests based upon the MantidSystemTest
# class. This adds the system test modules (files), rather than the classes, but will run every class in the module.
function(PYSYSTEMTEST_ADD_TEST _test_src_dir _testname_prefix)
  if(NOT PYSYSTEMTEST_RUNNER)
    set(_systest_runner ${CMAKE_SOURCE_DIR}/Testing/SystemTests/scripts/systestrunner.py)
  else()
    set(_systest_runner ${PYSYSTEMTEST_RUNNER})
  endif()
  py_add_test("SystemTest" ${_systest_runner} ${ARGV})

endfunction()

# PY_ADD_TEST is used by the above test-adding methods. It SHOULD NOT be used directly in CMakeLists.txt files. Use
# PYSYSTEMTEST_ADD_TEST or PYUNITTEST_ADD_TEST instead.
function(PY_ADD_TEST _test_type _test_runner_module _test_src_dir _testname_prefix)
  # Property for the module directory
  if(CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR MATCHES "Xcode")
    set(_module_dir ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
  else()
    set(_module_dir ${CMAKE_BINARY_DIR}/bin)
  endif()

  # Environment
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(_python_path ${_test_src_dir};${PYUNITTEST_PYTHONPATH_EXTRA};$ENV{PYTHONPATH};${_module_dir})
    # cmake list separator and Windows environment separator are the same so escape the cmake one
    string(REPLACE ";" "\\;" _python_path "${_python_path}")
  else()
    string(REPLACE ";" ":" _python_path "${PYUNITTEST_PYTHONPATH_EXTRA}")
    set(_python_path ${_test_src_dir}:${_python_path}:$ENV{PYTHONPATH}:${_module_dir})
  endif()
  # Define the environment
  list(APPEND _test_environment "PYTHONPATH=${_python_path}")
  if(PYUNITTEST_QT_API)
    list(APPEND _test_environment "QT_API=${PYUNITTEST_QT_API}")
  endif()

  # The scripts need jemalloc to be resolved to the runtime library as the plain .so symlink is only present when a
  # -dev/-devel package is present
  if(JEMALLOCLIB_FOUND)
    get_filename_component(JEMALLOC_RUNTIME_LIB ${JEMALLOC_LIBRARIES} REALPATH)
    # We only want to use the major version number
    string(REGEX REPLACE "([0-9]+)\.[0-9]+\.[0-9]+$" "\\1" JEMALLOC_RUNTIME_LIB ${JEMALLOC_RUNTIME_LIB})
  endif()

  # set preload as jemalloc, unless if using address sanitizer as this confuses things
  if(NOT WITH_ASAN)
    set(LOCAL_PRELOAD ${JEMALLOC_RUNTIME_LIB})
    if(LD_PRELOAD)
      set(LOCAL_PRELOAD ${LOCAL_PRELOAD}:$ENV{LD_PRELOAD})
    endif()
    list(APPEND _test_environment "LD_PRELOAD=${LOCAL_PRELOAD}")
  endif()

  # Check if this is a PR build.
  if(PR_JOB)
    set(_pr_flag "True")
  else()
    set(_pr_flag "False")
  endif()

  # Add all of the individual tests so that they can be run in parallel
  foreach(part ${ARGN})
    set(_filename ${part})
    get_filename_component(_suitename ${part} NAME_WE)
    # We duplicate the suitename so that it matches the junit output name
    set(_pyunit_separate_name "${_testname_prefix}.${_suitename}.${_suitename}")
    add_test(NAME ${_pyunit_separate_name}
             COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin/Testing" ${Python_EXECUTABLE}
                     ${_test_runner_module} ${_test_src_dir}/${_filename} ${_pr_flag}
    )
    # Set the PYTHONPATH so that the built modules can be found
    set_tests_properties(
      ${_pyunit_separate_name} PROPERTIES ENVIRONMENT "${_test_environment}" TIMEOUT ${TESTING_TIMEOUT} LABELS
                                          ${_test_type}
    )
    if(PYUNITTEST_RUN_SERIAL)
      set_tests_properties(${_pyunit_separate_name} PROPERTIES RUN_SERIAL 1)
    endif()
  endforeach(part ${ARGN})
endfunction()

# Defines a macro to check that each file contains a call to unittest.main() The arguments should be the source
# directory followed by the test files as list, e.g. check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_FILES} )
#
function(CHECK_TESTS_VALID _source_dir)
  set(_invalid_files)
  foreach(_test ${ARGN})
    file(STRINGS "${_source_dir}/${_test}" matches REGEX "unittest.main\(\)")
    if(NOT matches)
      set(_invalid_files "${_invalid_files}:${_test}")
    endif()
  endforeach()
  if(_invalid_files)
    set(_error
        "The following Python unit tests in ${_source_dir} do not contain a call to 'unittest.main()':
${_invalid_files}
Add the following line to end of the test files:
if __name__ == '__main__':  unittest.main()"
    )
    message(FATAL_ERROR ${_error})
  endif()
endfunction()
