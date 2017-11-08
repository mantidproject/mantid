# This file manages building/installation of the mantidqt and mantidqtpython
# Python wrappers.
#

# Legacy wrappers for MantidPlot
add_subdirectory ( mantidqtpython )

# All source files
set ( PYTHON_SOURCE_FILES
  mantidqt/__init__.py
)

# Copy to build directory and compile
set ( _pyc_files )
foreach ( _it ${PYTHON_SOURCE_FILES} )
  get_filename_component ( _directory ${_it} DIRECTORY )
  get_filename_component ( _filename_we ${_it} NAME_WE )
  set ( _pyc_out ${PYTHON_MODULE_OUTPUT_DIRECTORY}/${_directory}/${_filename_we}.pyc )
  list ( APPEND _pyc_files "${_pyc_out}" )
  set ( _pyc_out_no_gen_expr ${PYTHON_MODULE_OUTPUT_DIRECTORY_NO_GEN_EXPR}/${_directory}/${_filename_we}.pyc )
  add_custom_command(
    OUTPUT "${_pyc_out_no_gen_expr}"
    # copy
    COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${_it}"
            "${PYTHON_MODULE_OUTPUT_DIRECTORY}/${_it}"
    # compile
    COMMAND ${PYTHON_EXECUTABLE} -m compileall -q
            "${PYTHON_MODULE_OUTPUT_DIRECTORY}/${_it}"

    COMMENT "Compiling ${_it}"
    DEPENDS
      ${_it}
  )
endforeach()

# Target
add_custom_target ( mantidqt ALL DEPENDS ${_pyc_files} SOURCES ${PYTHON_SOURCE_FILES} )

set ( PYTHON_TEST_FILES
  mantidqt/test/import_test.py
)

# Tests
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} mantidqt ${PYTHON_TEST_FILES} )

# Not installed yet...
