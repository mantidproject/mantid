
###########################################################################
# Add OpenMP flags here
###########################################################################
if ( OPENMP_FOUND )
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}" )
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}" )
endif ()

###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################

include ( UseSystemQt4 )
include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

# Any MantidQt package can depend on API, so add the includes to the path here
include_directories ( API/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/API )
add_subdirectory ( API )

# Anything EXCEPT API can depend on MantidWidgets
include_directories ( MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/MantidWidgets )
add_subdirectory ( MantidWidgets )

add_subdirectory ( CustomDialogs )
add_subdirectory ( CustomInterfaces )
add_subdirectory ( SliceViewer )
add_subdirectory ( SpectrumViewer )
add_subdirectory ( RefDetectorViewer)
add_subdirectory ( Factory )
add_subdirectory ( Python )
add_subdirectory ( DesignerPlugins )

###########################################################################
# Add a custom target to build all of the MantidQt packages
###########################################################################

if ( MSVC_IDE )
  # Add to the 'MantidQt' group in VS
  set_property ( TARGET MantidQtAPI PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidWidgets PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET CustomDialogs PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET CustomInterfaces PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET DesignerPlugins PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtSliceViewer PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtImageViewer PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtFactory PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET mantidqtpython PROPERTY FOLDER "MantidQt" )
else ()
  add_custom_target ( MantidQt DEPENDS MantidQtAPI MantidWidgets 
                                       CustomDialogs CustomInterfaces
                                       MantidQtSliceViewer MantidQtImageViewer
                                       MantidQtFactory mantidqtpython
  )
endif ()






