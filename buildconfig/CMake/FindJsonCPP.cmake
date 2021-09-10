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


if(WIN32)
  string( REPLACE ".lib" ".dll" JSONCPP_LIBRARY_DLL       "${JSONCPP_LIBRARY}" )
  string( REPLACE ".lib" ".dll" JSONCPP_LIBRARY_DEBUG_DLL "${JSONCPP_LIBRARY_DEBUG}" )
  get_filename_component(JSONCPP_LIBRARY_DLL ${JSONCPP_LIBRARY_DLL} NAME)
  get_filename_component(JSONCPP_LIBRARY_DEBUG_DLL ${JSONCPP_LIBRARY_DEBUG_DLL} NAME)
  find_file(JSONCPP_LIBRARY_DLL PATH_SUFFIXES bin/)
  find_file(JSONCPP_LIBRARY_DEBUG_DLL PATH_SUFFIXES bin/)
endif()

if( JSONCPP_FOUND AND NOT TARGET JsonCPP::jsoncpp )
if( JSONCPP_LIBRARY_DLL)

    # Windows systems with dll libraries.
    add_library( JsonCPP::jsoncpp SHARED IMPORTED )

    # Windows with dlls, but only Release libraries.
    set_target_properties(JsonCPP::jsoncpp PROPERTIES
      IMPORTED_LOCATION_RELEASE         "${JSONCPP_LIBRARY_DLL}"
      IMPORTED_IMPLIB                   "${JSONCPP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES     "${JSONCPP_INCLUDE_DIR}"
      IMPORTED_CONFIGURATIONS           Release
      IMPORTED_LINK_INTERFACE_LANGUAGES "CXX" )


    # If we have both Debug and Release libraries
    if(JSONCPP_LIBRARY_DEBUG_DLL)
      set_property( TARGET JsonCPP::jsoncpp APPEND PROPERTY IMPORTED_CONFIGURATIONS Debug )
      set_target_properties( JsonCPP::jsoncpp PROPERTIES
        IMPORTED_LOCATION_DEBUG           "${JSONCPP_LIBRARY_DEBUG_DLL}"
        IMPORTED_IMPLIB_DEBUG             "${JSONCPP_LIBRARY_DEBUG}"
        )
endif()
else()
  add_library(JsonCPP::jsoncpp UNKNOWN IMPORTED)
  set_target_properties(JsonCPP::jsoncpp PROPERTIES
  IMPORTED_LOCATION                 "${JSONCPP_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES     "${JSONCPP_INCLUDE_DIR}"
  IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
  )
endif()
endif()
