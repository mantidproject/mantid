# - Find jsoncpp include dirs and libraries
# Use this module by invoking find_package with the form:
#  find_package(JsonCPP [required] [quiet] )
#
# The module sets the following variables 
#  JSONCPP_FOUND          - True if headers and libraries were found
#  JSONCPP_INCLUDE_DIR    - jsoncpp include directories
#  JSONCPP_LIBRARY        - library files for linking (optimised version)
#  JSONCPP_LIBRARY_DEBUG  - library files for linking (debug version)
#  JSONCPP_LIBRARIES      - All required libraries, including the configuration type

# Headers
find_path ( JSONCPP_INCLUDE_DIR json/reader.h
            PATH_SUFFIXES jsoncpp )

# Libraries
find_library ( JSONCPP_LIBRARY NAMES jsoncpp ) # optimized
find_library ( JSONCPP_LIBRARY_DEBUG NAMES jsoncpp_d ) # debug

if ( JSONCPP_LIBRARY AND JSONCPP_LIBRARY_DEBUG )
  set ( JSONCPP_LIBRARIES optimized ${JSONCPP_LIBRARY} debug ${JSONCPP_LIBRARY_DEBUG} )
else()
  set ( JSONCPP_LIBRARIES ${JSONCPP_LIBRARY} )
endif()

set(_JSONCPP_VERSION_ARGS)
if (EXISTS "${JSONCPP_INCLUDE_DIR}/json/version.h")
  file(STRINGS "${JSONCPP_INCLUDE_DIR}/json/version.h" _JSONCPP_VERSION_CONTENTS REGEX "JSONCPP_VERSION_[A-Z]+")
  foreach (_JSONCPP_VERSION_PART MAJOR MINOR PATCH)
    string(REGEX REPLACE ".*# *define +JSONCPP_VERSION_${_JSONCPP_VERSION_PART} +([0-9]+).*" "\\1" JSONCPP_VERSION_${_JSONCPP_VERSION_PART} "${_JSONCPP_VERSION_CONTENTS}")
  endforeach ()

  set(JSONCPP_VERSION_STRING "${JSONCPP_VERSION_MAJOR}.${JSONCPP_VERSION_MINOR}.${JSONCPP_VERSION_PATCH}")

  set(_JSONCPP_VERSION_ARGS VERSION_VAR JSONCPP_VERSION_STRING)
endif ()

# Handle the QUIETLY and REQUIRED arguments and set JSONCPP_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( JsonCPP REQUIRED_VARS JSONCPP_INCLUDE_DIR JSONCPP_LIBRARIES ${_JSONCPP_VERSION_ARGS} )

# Advanced variables
mark_as_advanced ( JSONCPP_INCLUDE_DIR JSONCPP_LIBRARIES )


