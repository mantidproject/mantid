#
# PYUNITTEST_ADD_TEST_TWO (public macro to add unit tests)
#   Adds a set of python tests based upon the unittest module
#   Parameters:
#       _test_src_dir :: The directory where the src files reside
#       _testname_prefix :: A prefix for each test that is added to ctest, the name will be
#                           ${_testname_prefix}_TestName
#       ${ARGN} :: List of test files
macro ( PYUNITTEST_ADD_TEST_TWO _test_src_dir _testname_prefix )
  # The working directory when running
  set ( _running_dir "${CMAKE_BINARY_DIR}/bin" )
  # Add all of the individual tests so that they can be run in parallel
  foreach ( part ${ARGN} )
    get_filename_component( _filename ${part} NAME )
    get_filename_component( _suitename ${part} NAME_WE )
    set ( _pyunit_separate_name "${_testname_prefix}_${_suitename}" )
    add_test ( NAME ${_pyunit_separate_name}
               COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin"
               ${PYTHON_EXECUTABLE} -m unittest discover
                                    --start-directory=${_test_src_dir}
                                    --pattern=${_filename} )
  endforeach ( part ${ARGN} )

endmacro ( PYUNITTEST_ADD_TEST_TWO )


macro ( PYUNITTEST_ADD_TEST _pyunit_testname_file )
  # decide where to copy the unit tests
  get_filename_component ( _pyunit_testname ${_pyunit_testname_file} NAME_WE )
  set ( _pyunit_outputdir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_testname} )

  # Add a special target to prepare the output directory. It needs to be run everytime the
  # main target is run so we can flush out any tests that may have been removed from the source tree but
  # still reside in the build
  add_custom_target ( Prepare${_pyunit_testname_file}
                      COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_testname_file}
                      COMMAND ${CMAKE_COMMAND} -E remove_directory ${_pyunit_outputdir}
                      COMMAND ${CMAKE_COMMAND} -E make_directory ${_pyunit_outputdir}
                      COMMAND ${CMAKE_COMMAND} -E touch ${_pyunit_outputdir}/__init__.py 
                    )

  # Copy the unit test files
  set ( _pyunit_testfiles "" )
  foreach (part ${ARGN})
    get_filename_component(_pyunit_file ${part} NAME)
    add_custom_command ( OUTPUT ${_pyunit_outputdir}/${_pyunit_file}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${part}
                     COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/${part}
                         ${_pyunit_outputdir}/${_pyunit_file} )
    set ( _pyunit_testfiles ${_pyunit_testfiles} ${_pyunit_outputdir}/${_pyunit_file} )
  endforeach (part ${ARGN})

  # The TESTHELPER_PY_FILES variable can be used outside of this macro
  # to include any helper classes that are not run through the python unittest generator
  set ( _testhelper_files "" )
  foreach (part ${TESTHELPER_PY_FILES})
    get_filename_component(_testhelper_file ${part} NAME)
    add_custom_command ( OUTPUT ${_pyunit_outputdir}/${_testhelper_file} ${_pyunit_outputdir}/__init__.py
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${part}
                     COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/${part}
                         ${_pyunit_outputdir}/${_testhelper_file} )
    set ( _testhelper_files ${_testhelper_files} ${_pyunit_outputdir}/${_testhelper_file} )
  endforeach (part ${ARGN})

  # Main test target
  add_custom_target ( ${_pyunit_testname_file}
                      DEPENDS  ${_pyunit_testfiles} ${_testhelper_files}
                      COMMAND ${PYTHON_EXECUTABLE} ${PYUNITTEST_GEN_EXEC}
                              -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_testname_file}
                              -d ${_pyunit_outputdir}
                              --xmlrunner=${PYUNITTEST_XMLRUNNER}
                              --python="${PYTHON_EXECUTABLE}" )
  # Make sure the directory is flushed each time before it is rebuilt
  add_dependencies( ${_pyunit_testname_file} Prepare${_pyunit_testname_file} )
  set_source_files_properties( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_testname_file}
                               PROPERTIES GENERATED true)

  if( MSVC )
    # We need to call the debug executable for the debug builds
    add_test (NAME ${_pyunit_testname}_py_Debug CONFIGURATIONS Debug
              COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin/Debug"
              ${PYTHON_EXECUTABLE_DEBUG} -B $<TARGET_FILE_DIR:PythonAPI>/${_pyunit_testname_file} )
    add_test (NAME ${_pyunit_testname}_py CONFIGURATIONS Release
              COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin/Release"
              ${PYTHON_EXECUTABLE} -B $<TARGET_FILE_DIR:PythonAPI>/${_pyunit_testname_file} )
  else()
    add_test (NAME ${_pyunit_testname}_py
              COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin"
              ${PYTHON_EXECUTABLE} -B $<TARGET_FILE_DIR:PythonAPI>/${_pyunit_testname_file} )
  endif()

  # add all of the individual tests - this introduces a race condition
  #foreach (part ${ARGN})
  #  get_filename_component(_suitename ${part} NAME_WE)
  #  set (_pyunit_separate_name "${_pyunit_testname}_${_suitename}")
  #  add_test ( NAME ${_pyunit_separate_name}
  #             COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin"
  #                 ${PYTHON_EXECUTABLE} ${_pyunit_testname_file} ${_suitename} )
  #endforeach (part ${ARGN})
endmacro ( PYUNITTEST_ADD_TEST )

#=============================================================
# main()
#=============================================================

# find the driver script
find_program ( PYUNITTEST_GEN_EXEC pyunit_gen.py
               PATHS ${PROJECT_SOURCE_DIR}/TestingTools/pyunit_gen
                     ${PROJECT_SOURCE_DIR}/../TestingTools/pyunit_gen )

# determine where the xmlrunner lives
find_path ( PYUNITTEST_XMLRUNNER xmlrunner/__init__.py
            PATHS ${PROJECT_SOURCE_DIR}/TestingTools/unittest-xml-reporting/src/
                  ${PROJECT_SOURCE_DIR}/../TestingTools/unittest-xml-reporting/src/ )

# let people know whether or not it was found
if (PYUNITTEST_GEN_EXEC)
  set ( PYUNITTEST_FOUND TRUE )
else ()
  set ( PYUNITTEST_FOUND FALSE )
endif ()

mark_as_advanced ( PYUNITTEST_GEN_EXEC PYUNITTEST_XMLRUNNER )
