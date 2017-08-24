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
  set_property ( TARGET MantidQtWidgetsCommonQt4 PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsFactoryQt4 PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsInstrumentViewQt4 PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsRefDetectorViewQt4 PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsSliceViewerQt4 PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsSpectrumViewerQt4 PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsPluginsDesigner PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET MantidQtWidgetsPluginsAlgorithmDialogs PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET mantidqtpython PROPERTY FOLDER "MantidQt" )
else ()
  add_custom_target ( MantidQt DEPENDS MantidQtWidgetsCommonQt4
                                       MantidQtWidgetsFactoryQt4
                                       MantidQtWidgetsInstrumentViewQt4
                                       MantidQtWidgetsSliceViewerQt4
                                       MantidQtWidgetsSpectrumViewerQt4
                                       MantidQtWidgetsRefDetectorView
                                       MantidQtWidgetsPluginsAlgorithmDialogs
                                       mantidqtpython
  )
endif ()
