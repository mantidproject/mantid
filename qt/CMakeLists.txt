###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################
find_package ( QScintilla REQUIRED )

include ( UseQt4 )
include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

# Utilities for defining targets
include (QtTargetFunctions)

# Any MantidQt package can depend on widgets/common, so add the includes to the path here
include_directories ( widgets/common/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/widgets/common )
add_subdirectory ( widgets )
add_subdirectory ( python )
add_subdirectory ( scientific_interfaces )
if ( MAKE_VATES )
  add_subdirectory ( paraview_ext )
endif ( MAKE_VATES )

###########################################################################
# Add a custom target to build all of the MantidQt packages
###########################################################################

if ( MSVC_IDE )
#  # Add to the 'MantidQt' group in VS
  set_property ( TARGET MantidQtWidgetsCommon PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsInstrumentView PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsSliceViewer PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsSpectrumViewer PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsRefDetectorView PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsFactoryQt4 PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsPluginsDesigner PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsPluginsAlgorithmDialogs PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET mantidqtpython PROPERTY FOLDER "MantidQt" )
else ()
  add_custom_target ( MantidQt DEPENDS MantidQtWidgetsCommon 
                                       MantidQtWidgetsSliceViewer
                                       MantidQtWidgetsSpectrumViewer
                                       MantidQtWidgetsRefDetectorView
                                       MantidQtWidgetsFactory
                                       MantidQtWidgetsPluginsAlgorithmDialogs
                                       mantidqtpython
  )
endif ()
