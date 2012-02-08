###############################################################################
# - Attempt to find Qwt libraries and include files. 
# QWT_INCLUDE_DIR where to find qwt_plot.h, etc.
# QWT_LIBRARIES libraries to link against
# QWT_FOUND If false, do not try to use Qwt
# QWT_VERSION Sets a string containing the version number parsed from 
#             qwt_global.h
###############################################################################

find_path ( QWT_INCLUDE_DIR qwt.h 
            /usr/include/qwt-qt4 /usr/include/qwt 
	    ${CMAKE_INCLUDE_PATH}/qwt 
)
find_library ( QWT_LIBRARY NAMES qwt-qt4 qwt )
find_library ( QWT_LIBRARY_DEBUG qwtd )

# handle the QUIETLY and REQUIRED arguments and set QWT_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Qwt DEFAULT_MSG QWT_LIBRARY QWT_INCLUDE_DIR )

if ( QWT_INCLUDE_DIR )
  # Look for line in qwt_global.h containing version string
  file ( STRINGS ${QWT_INCLUDE_DIR}/qwt_global.h QWT_VERSION 
         REGEX "^#define[ \t]+QWT_VERSION_STR[ \t]+\"[0-9]+.[0-9]+.[0-9]+\"$" )
  if ( NOT QWT_VERSION )
    message ( WARNING "Unrecognized Qwt version, cannot find QWT_VERSION_STR in qwt_global.h" )
    set ( QWT_VERSION "0.0.0" )
  else()
    # Hack off the portion up to and including the first double quote 
    string( REGEX REPLACE "^#define[ \t]+QWT_VERSION_STR[ \t]+\"" "" QWT_VERSION ${QWT_VERSION} )
    # Hack off the portion from the second double quote to the end of the line
    string( REGEX REPLACE "\"$" "" QWT_VERSION ${QWT_VERSION} )
  endif()

  if ( QWT_LIBRARY_DEBUG )
    set( QWT_LIBRARIES optimized ${QWT_LIBRARY} debug ${QWT_LIBRARY_DEBUG} )
  else ()
    set( QWT_LIBRARIES ${QWT_LIBRARY} )
  endif ()
endif()

mark_as_advanced ( QWT_INCLUDE_DIR QWT_LIBRARY QWT_LIBRARY_DEBUG )
