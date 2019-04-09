# PYUNITTEST_ADD_TEST (public macro to add unit tests)
#   Adds a set of python tests based upon the unittest module
#
#   The variable PYUNITTEST_PYTHONPATH_EXTRA can be defined with
#   extra paths to add to PYTHONPATH during the tests
#   Parameters:
#       _test_src_dir :: A base directory when added to the relative test paths gives
#                        an absolute path to that test. This directory is added to the
#                        PYTHONPATH when tests are executed
#       _testname_prefix :: A prefix for each test that is added to ctest, the name will be
#                           ${_testname_prefix}_TestName
#       ${ARGN} :: List of test files
function ( PYUNITTEST_ADD_TEST _test_src_dir _testname_prefix )
  # Property for the module directory
  set ( _working_dir ${CMAKE_BINARY_DIR}/bin/Testing )
  if ( CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR MATCHES "Xcode" )
    set ( _module_dir ${CMAKE_BINARY_DIR}/bin/$<CONFIG> )
  else()
    set ( _module_dir ${CMAKE_BINARY_DIR}/bin )
  endif()

  set ( _test_runner ${_module_dir}/mantidpython )
  if ( WIN32 )
    set ( _test_runner ${_test_runner}.bat )
  endif ()

  if ( NOT PYUNITTEST_RUNNER )
    set ( _test_runner_module ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/test/testhelpers/testrunner.py )
  else ()
    set ( _test_runner_module ${PYUNITTEST_RUNNER} )
  endif()

  # Environment
  if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set ( _python_path ${_test_src_dir};${PYUNITTEST_PYTHONPATH_EXTRA};$ENV{PYTHONPATH} )
    # cmake list separator and Windows environment separator are the same so escape the cmake one
    string ( REPLACE ";" "\\;" _python_path "${_python_path}" )
  else()
    string ( REPLACE ";" ":" _python_path "${PYUNITTEST_PYTHONPATH_EXTRA}" )
    set ( _python_path ${_test_src_dir}:${_python_path}:$ENV{PYTHONPATH} )
  endif()
  # Define the environment
  list ( APPEND _test_environment "PYTHONPATH=${_python_path}" )
  if ( PYUNITTEST_QT_API )
    list ( APPEND _test_environment "QT_API=${PYUNITTEST_QT_API}" )
  endif()

  # Add all of the individual tests so that they can be run in parallel
  foreach ( part ${ARGN} )
    set ( _filename ${part} )
    get_filename_component( _suitename ${part} NAME_WE )
    # We duplicate the suitename so that it matches the junit output name
    set ( _pyunit_separate_name "${_testname_prefix}.${_suitename}.${_suitename}" )
    add_test ( NAME ${_pyunit_separate_name}
               COMMAND ${_test_runner} --classic ${_test_runner_module} ${_test_src_dir}/${_filename} )
    # Set the PYTHONPATH so that the built modules can be found
    set_tests_properties ( ${_pyunit_separate_name} PROPERTIES
                           WORKING_DIRECTORY ${_working_dir}
                           ENVIRONMENT "${_test_environment}"
                           TIMEOUT ${TESTING_TIMEOUT} )
    if ( PYUNITTEST_RUN_SERIAL )
      set_tests_properties ( ${_pyunit_separate_name} PROPERTIES
                             RUN_SERIAL 1 )
    endif ()
  endforeach ( part ${ARGN} )
endfunction ()

# Defines a macro to check that each file contains a call to unittest.main()
# The arguments should be the source directory followed by the test files as
# list, e.g. check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_FILES} )
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
    set(
      _error
      "The following Python unit tests in ${_source_dir} do not contain a call to 'unittest.main()':
${_invalid_files}
Add the following line to end of the test files:
if __name__ == '__main__':  unittest.main()")
    message(FATAL_ERROR ${_error})
  endif()
endfunction()
