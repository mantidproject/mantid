###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################

include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )
include_directories ( API/inc )
add_subdirectory ( API )
add_subdirectory ( MantidWidgets )
add_subdirectory ( CustomDialogs )
add_subdirectory ( CustomInterfaces )
add_subdirectory ( DesignerPlugins )
add_subdirectory ( SliceViewer )

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
else ()
  add_custom_target ( MantidQt DEPENDS MantidQtAPI MantidWidgets 
                                       CustomDialogs CustomInterfaces MantidQtSliceViewer
  )
endif ()
