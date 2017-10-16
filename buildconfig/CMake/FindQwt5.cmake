###############################################################################
# Attempt to find Qwt libraries and include files.
# Set
#   QWT5_INCLUDE_DIR: where to find qwt_plot.h, etc.
#   QWT5_LIBRARIES:   libraries to link against
#   QWT5_VERSION:     a string containing the version number
###############################################################################
# Hacky way to get CMake to find the updated mantidlibs version of qwt 
set ( MANTIDLIBS mantidlibs34 )
find_path ( QWT5_INCLUDE_DIR qwt.h
            PATHS /opt/rh/${MANTIDLIBS}/root/usr/include /opt/include /usr/local/include /usr/include ${CMAKE_INCLUDE_PATH}
            PATH_SUFFIXES qwt5 qwt5-qt4 qwt-qt4 qwt
            NO_DEFAULT_PATH )
find_library ( QWT5_LIBRARY NAMES qwt5-qt4 qwt-qt4 qwt5 qwt HINTS /opt/rh/${MANTIDLIBS}/root/usr/lib64 )
find_library ( QWT5_LIBRARY_DEBUG NAMES qwtd qwtd5 )

# in REQUIRED mode: terminate if one of the above find commands failed
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Qwt DEFAULT_MSG QWT5_LIBRARY QWT5_INCLUDE_DIR )

# Parse version string from qwt_global.h
file ( STRINGS ${QWT5_INCLUDE_DIR}/qwt_global.h QWT5_VERSION
       REGEX "^#define[ \t]+QWT_VERSION_STR[ \t]+\"[0-9]+.[0-9]+.[0-9]+(.*)?\"$" )
if ( NOT QWT5_VERSION )
    message ( FATAL_ERROR "Unrecognized Qwt version (cannot find QWT5_VERSION_STR in qwt_global.h)" )
endif()
#   hack off the portion up to and including the first double quote
string( REGEX REPLACE "^#define[ \t]+QWT_VERSION_STR[ \t]+\"" "" QWT5_VERSION ${QWT5_VERSION} )
#   hack off the portion from the second double quote to the end of the line
string( REGEX REPLACE "\"$" "" QWT5_VERSION ${QWT5_VERSION} )

if ( QWT5_LIBRARY_DEBUG )
  set( QWT5_LIBRARIES optimized ${QWT5_LIBRARY} debug ${QWT5_LIBRARY_DEBUG} )
else ()
  set( QWT5_LIBRARIES ${QWT5_LIBRARY} )
endif ()

mark_as_advanced ( QWT5_INCLUDE_DIR QWT5_LIBRARY QWT5_LIBRARY_DEBUG )
