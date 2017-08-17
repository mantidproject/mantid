#
# This module attempts to find the Python Sphinx
# documentation generator
#
# If the module is found the following variables are
# set:
#  SPHINX_FOUND
#  SPHINX_PACKAGE_DIR
#
#=============================================================
# main()
#=============================================================


FIND_FILE(_find_sphinx_py FindSphinx.py PATHS ${CMAKE_MODULE_PATH})

# import sphinx-build to attempt to get the version
execute_process (COMMAND ${PYTHON_EXECUTABLE} ${_find_sphinx_py} OUTPUT_VARIABLE sphinx_output
                                                                 RESULT_VARIABLE sphinx_result)
list(GET sphinx_output 0 version_string)
list(GET sphinx_output 1 SPHINX_PACKAGE_DIR)

if (${sphinx_result} STREQUAL "0" AND version_string)
  # chop out the version number
  string (REGEX REPLACE ".*([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" SPHINX_VERSION ${version_string})
endif ()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_PACKAGE_DIR)

