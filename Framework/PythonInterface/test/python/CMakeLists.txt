# This directory (and subdirectories) contains Python tests for each subpackage
# and the PythonAlgorithms

if(PYUNITTEST_FOUND)

  # Defines a macro to check that each file contains a call to unittest.main()
  # The arguments should be the source directory followed by the test files as
  # list, e.g. check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_FILES} )
  #
  macro(CHECK_TESTS_VALID _source_dir)
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
  endmacro()

  add_subdirectory(mantid)
  add_subdirectory(plugins)
endif()
