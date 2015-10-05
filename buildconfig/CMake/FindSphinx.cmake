# 
# This module attempts to find the Python Sphinx
# documentation generator
#
# If the module is found the following variables are
# set:
#  SPHINX_FOUND
#  SPHINX_EXECUTABLE
#
#=============================================================
# main()
#=============================================================

find_program( SPHINX_EXECUTABLE NAME sphinx-build
  PATHS ${CMAKE_LIBRARY_PATH}/Python27/Scripts
  PATH_SUFFIXES bin
  DOC "Sphinx documentation generator"
)

if (SPHINX_EXECUTABLE)
    # run sphinx-build to attempt to get the version
    execute_process (COMMAND ${SPHINX_EXECUTABLE} --version
                     OUTPUT_STRIP_TRAILING_WHITESPACE
                     OUTPUT_VARIABLE version_string
                     ERROR_VARIABLE version_error_string
                     ERROR_STRIP_TRAILING_WHITESPACE)

    # if it wasn't successful it is hiding in stderr
    if (NOT version_string)
        if ( version_error_string )
            string (REGEX REPLACE "\n" ";" version_string ${version_error_string})
            list (GET version_string 0 version_string)
        else ( version_error_string )
            set ( version_string "1.1.0" )
        endif ( version_error_string )
    endif (NOT version_string)

    # chop out the version number
    string (REGEX REPLACE ".*([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" SPHINX_VERSION ${version_string})
endif (SPHINX_EXECUTABLE)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_EXECUTABLE )

mark_as_advanced ( SPHINX_EXECUTABLE )
