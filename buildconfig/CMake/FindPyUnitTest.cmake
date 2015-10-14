#
# PYUNITTEST_ADD_TEST (public macro to add unit tests)
#   Adds a set of python tests based upon the unittest module
#   Parameters:
#       _test_src_dir :: The directory where the src files reside
#       _testname_prefix :: A prefix for each test that is added to ctest, the name will be
#                           ${_testname_prefix}_TestName
#       ${ARGN} :: List of test files
macro ( PYUNITTEST_ADD_TEST _test_src_dir _testname_prefix )
  # Property for the module directory
  if ( MSVC )
    set ( _module_dir ${CMAKE_BINARY_DIR}/bin/Release )
    set ( _module_dir_debug ${CMAKE_BINARY_DIR}/bin/Debug )
    set ( _working_dir ${_module_dir} )
    set ( _working_dir_debug ${_module_dir_debug} )
  else()
    set ( _module_dir ${CMAKE_BINARY_DIR}/bin )
    set ( _working_dir ${_module_dir} )
  endif()

  # Add all of the individual tests so that they can be run in parallel
  foreach ( part ${ARGN} )
    get_filename_component( _filename ${part} NAME )
    get_filename_component( _suitename ${part} NAME_WE )
    set ( _pyunit_separate_name "${_testname_prefix}_${_suitename}" )

    if ( MSVC )
      # Debug builds need to call the debug executable
      add_test ( NAME ${_pyunit_separate_name}_Debug CONFIGURATIONS Debug
                 COMMAND ${PYTHON_EXECUTABLE_DEBUG} -B ${_test_src_dir}/${_filename} )
      # Set the PYTHONPATH so that the built modules can be found
      set_tests_properties ( ${_pyunit_separate_name}_Debug PROPERTIES
                             ENVIRONMENT "PYTHONPATH=${_module_dir_debug}"
                             WORKING_DIRECTORY ${_working_dir_debug}
                             TIMEOUT ${TESTING_TIMEOUT} )
      # Release
      add_test ( NAME ${_pyunit_separate_name} CONFIGURATIONS Release
                 COMMAND ${PYTHON_EXECUTABLE} -B ${_test_src_dir}/${_filename} )
      # Set the PYTHONPATH so that the built modules can be found
      set_tests_properties ( ${_pyunit_separate_name} PROPERTIES
                             ENVIRONMENT "PYTHONPATH=${_module_dir}"
                             WORKING_DIRECTORY ${_working_dir}
                             TIMEOUT ${TESTING_TIMEOUT} )
    else()
      add_test ( NAME ${_pyunit_separate_name}
                 COMMAND ${PYTHON_EXECUTABLE} -B ${_test_src_dir}/${_filename} )
      # Set the PYTHONPATH so that the built modules can be found
      set_tests_properties ( ${_pyunit_separate_name} PROPERTIES
                             ENVIRONMENT "PYTHONPATH=${_module_dir}"
                             WORKING_DIRECTORY ${_working_dir}
                             TIMEOUT ${TESTING_TIMEOUT} )
    endif()
  endforeach ( part ${ARGN} )
endmacro ( PYUNITTEST_ADD_TEST )

#=============================================================
# main()
#=============================================================

# find the driver script
find_program ( PYUNITTEST_GEN_EXEC pyunit_gen.py
               PATHS ${PROJECT_SOURCE_DIR}/Testing/Tools/pyunit_gen
                     ${PROJECT_SOURCE_DIR}/../Testing/Tools/pyunit_gen )

# determine where the xmlrunner lives
find_path ( PYUNITTEST_XMLRUNNER xmlrunner/__init__.py
            PATHS ${PROJECT_SOURCE_DIR}/Testing/Tools/unittest-xml-reporting/src/
                  ${PROJECT_SOURCE_DIR}/../Testing/Tools/unittest-xml-reporting/src/ )

# let people know whether or not it was found
if (PYUNITTEST_GEN_EXEC)
  set ( PYUNITTEST_FOUND TRUE )
else ()
  set ( PYUNITTEST_FOUND FALSE )
endif ()

mark_as_advanced ( PYUNITTEST_GEN_EXEC PYUNITTEST_XMLRUNNER )
