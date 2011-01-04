###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################

include_directories ( API/inc )
add_subdirectory ( API )
add_subdirectory ( MantidWidgets )
add_subdirectory ( CustomDialogs )
add_subdirectory ( CustomInterfaces )

###########################################################################
# Add a custom target to build all of the MantidQt packages
###########################################################################

add_custom_target ( MantidQt DEPENDS MantidQtAPI MantidWidgets 
                                     CustomDialogs CustomInterfaces
) 