# Utility functions to add targets that "build" from pure python
# sources

# Add rules to create a target that will copy and copy the
# given python sources to a given destination. The file names should
# be given relative to the current CMake sources list, e.g
#
#   set ( SRCS
#     mypkg/__init__.py
#   )
#   add_python_package ( TARGET_NAME mypkg
#     SRCS ${SRCS}
#     OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/mypkg
#   )
#
# will produce a directory in the specified location containing the listed
# files.
#
# Arguments:
#   TARGET_NAME: The name of the target
#   OUTPUT_DIR: The base directory for the copied and compiled files. Please
#               note that this is used with add_custom_command so cannot
#               contain generator expressions
#   SRCS: A list of python source files for this package. The paths must be
#         relative to the CMakeLists calling this function
#   TEST_SRCS: A list of test files to include
function ( add_python_package )
  set ( options )
  set ( oneValueArgs TARGET_NAME OUTPUT_DIR )
  set ( multiValueArgs SRCS TEST_SRCS )
  cmake_parse_arguments ( PARSED "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

  # Copy to build directory and compile
  set ( _all_src ${PARSED_SRCS};${PARSED_TEST_SRCS} )
  set ( _pyc_files )
  foreach ( _it  ${_all_src} )
    get_filename_component ( _directory ${_it} DIRECTORY )
    get_filename_component ( _filename_we ${_it} NAME_WE )
    set ( _pyc_out ${PARSED_OUTPUT_DIR}/${_directory}/${_filename_we}.pyc )
    list ( APPEND _pyc_files "${_pyc_out}" )
    add_custom_command (
      OUTPUT "${_pyc_out}"
      # copy
      COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
              "${CMAKE_CURRENT_SOURCE_DIR}/${_it}"
              "${PARSED_OUTPUT_DIR}/${_it}"
      # compile
      COMMAND ${PYTHON_EXECUTABLE} -m compileall -q
              "${PARSED_OUTPUT_DIR}/${_it}"

      COMMENT "Compiling ${_it}"
      DEPENDS
        ${_it}
    )
  endforeach()

  # Target
  add_custom_target ( ${PARSED_TARGET_NAME} ALL
    DEPENDS ${_pyc_files}
    SOURCES ${_all_src}
  )

 # Tests
 if ( PARSED_TEST_SRCS )
   pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
     ${PARSED_TARGET_NAME} ${PARSED_TEST_SRCS} )
 endif()
endfunction ()
