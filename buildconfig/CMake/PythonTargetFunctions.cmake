# Utility functions to add targets that "build" from pure python
# sources

# Add rules to create a target that will copy and copy the
# given python sources to a given destination. The file names should
# be given without any directory prefix, e.g
#
#   set ( SRCS
#     __init__.py
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
#   OUTPUT_DIR: The directory for the copied and compiled files
#   SRCS: A list of python source files for this package
function (add_python_package)
  set (options)
  set (oneValueArgs TARGET_NAME OUTPUT_DIR)
  set (multiValueArgs SRCS)
  cmake_parse_arguments (PARSED "${options}" "${oneValueArgs}"
                         "${multiValueArgs}" ${ARGN})

  foreach( _it ${PARSED_SRCS} )
    get_filename_component( _filename ${_it} NAME_WE )
    set ( _pyc ${_filename}.pyc )
    add_custom_command ( OUTPUT )
  endforeach()
  add_custom_target ( ${PARSED_TARGET_NAME} )
endfunction ()
