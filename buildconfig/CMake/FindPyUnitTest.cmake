#
# PYUNITTEST_ADD_TEST (public macro to add unit tests)
#   Adds a set of python tests based upon the unittest module
#
#   The variable PYUNITTEST_PYTHONPATH_EXTRA can be defined with
#   extra paths to add to PYTHONPATH during the tests
#   Parameters:
#       _test_src_dir_base :: A base directory when added to the relative test paths gives
#                             an absolute path to that test. This directory is added to the
#                             PYTHONPATH when tests are executed
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

  # get the absolute path to the PythonInterface directory
  # this is the root of the source for the mantid package
  get_filename_component(_pythoninterface_dir
                        "${CMAKE_SOURCE_DIR}/Framework/PythonInterface/"
                        ABSOLUTE
                        DIRECTORY)

  # Environment
  if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set ( _python_path ${PYTHON_XMLRUNNER_DIR};${_test_src_dir};${_pythoninterface_dir};${PYUNITTEST_PYTHONPATH_EXTRA};$ENV{PYTHONPATH} )
    # cmake list separator and Windows environment separator are the same so escape the cmake one
    string ( REPLACE ";" "\\;" _python_path "${_python_path}" )
  else()
    string ( REPLACE ";" ":" _python_path "${PYUNITTEST_PYTHONPATH_EXTRA}" )
    set ( _python_path ${PYTHON_XMLRUNNER_DIR}:${_test_src_dir}:${_python_path}:${_pythoninterface_dir}:$ENV{PYTHONPATH} )
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

#=============================================================
# main()
#=============================================================

# find the driver script
find_program ( PYUNITTEST_GEN_EXEC pyunit_gen.py
               PATHS ${PROJECT_SOURCE_DIR}/Testing/Tools/pyunit_gen
                     ${PROJECT_SOURCE_DIR}/../Testing/Tools/pyunit_gen )

# let people know whether or not it was found
if (PYUNITTEST_GEN_EXEC)
  set ( PYUNITTEST_FOUND TRUE )
else ()
  set ( PYUNITTEST_FOUND FALSE )
endif ()

mark_as_advanced ( PYUNITTEST_GEN_EXEC )
