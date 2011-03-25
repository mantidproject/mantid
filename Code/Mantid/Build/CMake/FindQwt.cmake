# - try to find Qwt libraries and include files
# QWT_INCLUDE_DIR where to find qwt_plot.h, etc.
# QWT_LIBRARIES libraries to link against
# QWT_FOUND If false, do not try to use Qwt

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

if ( QWT_FOUND )
  if ( QWT_LIBRARY_DEBUG )
    set( QWT_LIBRARIES optimized ${QWT_LIBRARY} debug ${QWT_LIBRARY_DEBUG} )
  else ()
    set( QWT_LIBRARIES ${QWT_LIBRARY} )
  endif ()
endif()

mark_as_advanced ( QWT_INCLUDE_DIR QWT_LIBRARY QWT_LIBRARY_DEBUG )
