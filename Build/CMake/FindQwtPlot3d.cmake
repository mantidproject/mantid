# - try to find QwtPlot3d libraries and include files
# QWTPLOT3D_INCLUDE_DIR where to find qwt3d_plot.h, etc.
# QWTPLOT3D_LIBRARIES libraries to link against
# QWTPLOT3D_FOUND If false, do not try to use Qwt

find_path ( QWTPLOT3D_INCLUDE_DIR qwt3d_plot.h 
                /usr/local/include /usr/include/qwtplot3d-qt4 /usr/include/qwtplot3d 
                ${CMAKE_INCLUDE_PATH}/qwtplot3d 
)
	  
if (APPLE)
  if (OSX_VERSION VERSION_GREATER 10.9 OR OSX_VERSION VERSION_EQUAL 10.9)
    include_directories("/opt/X11/include")
  endif()
endif()

find_library ( QWTPLOT3D_LIBRARY NAMES qwtplot3d-qt4 qwtplot3d )

find_library ( QWTPLOT3D_LIBRARY_DEBUG NAMES qwtplot3dd )

# handle the QUIETLY and REQUIRED arguments and set QWTPLOT3D_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( QwtPlot3d DEFAULT_MSG QWTPLOT3D_LIBRARY QWTPLOT3D_INCLUDE_DIR )

if ( QWTPLOT3D_FOUND )
  if ( QWTPLOT3D_LIBRARY_DEBUG )
    set( QWTPLOT3D_LIBRARIES optimized ${QWTPLOT3D_LIBRARY} debug ${QWTPLOT3D_LIBRARY_DEBUG} )
  else ()
    set( QWTPLOT3D_LIBRARIES ${QWTPLOT3D_LIBRARY} )
  endif ()
endif()

mark_as_advanced ( QWTPLOT3D_INCLUDE_DIR QWTPLOT3D_LIBRARY QWTPLOT3D_LIBRARY_DEBUG )
