###############################################################################
# Attempt to find Qwt libraries and include files.
# Set
#   QWT_INCLUDE_DIR: where to find qwt_plot.h, etc.
#   QWT_LIBRARIES:   libraries to link against
#   QWT_VERSION:     a string containing the version number
###############################################################################

find_path ( QWT_INCLUDE_DIR qwt.h 
            PATHS /opt/include /usr/local/include /usr/include ${CMAKE_INCLUDE_PATH}
            PATH_SUFFIXES qwt5 qwt5-qt4 qwt-qt4 qwt )
find_library ( QWT_LIBRARY NAMES qwt5-qt4 qwt-qt4 qwt5 qwt )
find_library ( QWT_LIBRARY_DEBUG qwtd )

# in REQUIRED mode: terminate if one of the above find commands failed
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Qwt DEFAULT_MSG QWT_LIBRARY QWT_INCLUDE_DIR )

# Parse version string from qwt_global.h
file ( STRINGS ${QWT_INCLUDE_DIR}/qwt_global.h QWT_VERSION 
       REGEX "^#define[ \t]+QWT_VERSION_STR[ \t]+\"[0-9]+.[0-9]+.[0-9]+\"$" )
if ( NOT QWT_VERSION )
    message ( FATAL_ERROR "Unrecognized Qwt version (cannot find QWT_VERSION_STR in qwt_global.h)" )
endif()
#   hack off the portion up to and including the first double quote 
string( REGEX REPLACE "^#define[ \t]+QWT_VERSION_STR[ \t]+\"" "" QWT_VERSION ${QWT_VERSION} )
#   hack off the portion from the second double quote to the end of the line
string( REGEX REPLACE "\"$" "" QWT_VERSION ${QWT_VERSION} )

if ( QWT_LIBRARY_DEBUG )
  set( QWT_LIBRARIES optimized ${QWT_LIBRARY} debug ${QWT_LIBRARY_DEBUG} )
else ()
  set( QWT_LIBRARIES ${QWT_LIBRARY} )
endif ()

mark_as_advanced ( QWT_INCLUDE_DIR QWT_LIBRARY QWT_LIBRARY_DEBUG )
