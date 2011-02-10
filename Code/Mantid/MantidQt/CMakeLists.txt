###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################

include_directories ( API/inc )
add_subdirectory ( API )
add_subdirectory ( MantidWidgets )
add_subdirectory ( CustomDialogs )
add_subdirectory ( CustomInterfaces )
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
else ()
  add_custom_target ( MantidQt DEPENDS MantidQtAPI MantidWidgets 
                                       CustomDialogs CustomInterfaces
  )
endif ()