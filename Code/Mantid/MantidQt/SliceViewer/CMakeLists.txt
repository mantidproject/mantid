set ( SRC_FILES
	src/ColorBarWidget.cpp
	src/CustomTools.cpp
	src/DimensionSliceWidget.cpp
	src/LineOverlay.cpp
	src/LineViewer.cpp
	src/QScienceSpinBox.cpp
	src/QwtRasterDataMD.cpp
	src/SliceViewer.cpp
	src/SliceViewerWindow.cpp
	src/SnapToGridDialog.cpp
	src/XYLimitsDialog.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	inc/MantidQtSliceViewer/ColorBarWidget.h
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/LineOverlay.h
	inc/MantidQtSliceViewer/LineViewer.h
	inc/MantidQtSliceViewer/QScienceSpinBox.h
	inc/MantidQtSliceViewer/QwtRasterDataMD.h
	inc/MantidQtSliceViewer/SliceViewer.h
	inc/MantidQtSliceViewer/SliceViewerWindow.h
	inc/MantidQtSliceViewer/SnapToGridDialog.h
	inc/MantidQtSliceViewer/XYLimitsDialog.h
)

set ( MOC_FILES 
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/LineOverlay.h
	inc/MantidQtSliceViewer/LineViewer.h
	inc/MantidQtSliceViewer/SliceViewer.h
	inc/MantidQtSliceViewer/SliceViewerWindow.h
	inc/MantidQtSliceViewer/ColorBarWidget.h
	inc/MantidQtSliceViewer/QScienceSpinBox.h
	inc/MantidQtSliceViewer/SnapToGridDialog.h
	inc/MantidQtSliceViewer/XYLimitsDialog.h
)

set ( UI_FILES 
	inc/MantidQtSliceViewer/DimensionSliceWidget.ui
	inc/MantidQtSliceViewer/SliceViewer.ui
	inc/MantidQtSliceViewer/LineViewer.ui
	inc/MantidQtSliceViewer/ColorBarWidget.ui
	inc/MantidQtSliceViewer/SnapToGridDialog.ui
	inc/MantidQtSliceViewer/XYLimitsDialog.ui
)

# Python unit tests
set ( TEST_PY_FILES 
	test/SliceViewerPythonInterfaceTest.py
	test/QwtTypesPythonInterfaceTest.py
)


include_directories ( inc )
include_directories ( ../../QtPropertyBrowser/src )


###########################################################################
# Main Library Target
###########################################################################

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

# --- Add icons qrc ------
qt4_add_resources ( RES_FILES icons/SliceViewerIcons.qrc )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 

include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_SLICEVIEWER )

# This creates the target library
add_library ( MantidQtSliceViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# ... and links to other required libs
target_link_libraries ( MantidQtSliceViewer MantidQtAPI QtPropertyBrowser ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################

if ( WIN32 )
# Does not work on windows for some reason
else ()

add_executable ( ColorBarWidgetDemo EXCLUDE_FROM_ALL ${ALL_SRC} src/main_ColorBarWidgetDemo.cpp ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS})
target_link_libraries ( ColorBarWidgetDemo MantidQtAPI QtPropertyBrowser ${QT_LIBRARIES} ${QWT_LIBRARIES} )


include_directories ( ../../Framework/MDEvents/inc )
include_directories ( ../../Framework/DataObjects/inc )
include_directories ( ../../Framework/Algorithms/inc )
include_directories ( ../../Framework/API/inc )
include_directories ( ../../Framework/DataHandling/inc )
include_directories ( ../../Framework/TestHelpers/inc )
include_directories ( ../../Framework/Geometry/inc )

# Extra, optional targets for demoing GUI elements
add_executable ( SliceViewerDemo EXCLUDE_FROM_ALL ${WIN_CONSOLE} MACOSX_BUNDLE ${ALL_SRC} src/main_SliceViewerDemo.cpp  
                           ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS} ${RES_FILES} ${MANTID_RC_FILE} )
target_link_libraries ( SliceViewerDemo 
                       ${CORE_MANTIDLIBS} MantidQtAPI MantidWidgets  MantidQtSliceViewer  MDEvents Geometry
                       QtPropertyBrowser ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${QWT_LIBRARIES} )

add_executable ( SliceViewerWindowDemo EXCLUDE_FROM_ALL ${WIN_CONSOLE} MACOSX_BUNDLE ${ALL_SRC} src/main_SliceViewerWindowDemo.cpp  
                           ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS} ${RES_FILES} ${MANTID_RC_FILE} )
target_link_libraries ( SliceViewerWindowDemo 
                       ${CORE_MANTIDLIBS} MantidQtAPI MantidWidgets  MantidQtSliceViewer  MDEvents Geometry
                       QtPropertyBrowser ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${QWT_LIBRARIES} )

add_executable ( LineViewerDemo EXCLUDE_FROM_ALL ${WIN_CONSOLE} MACOSX_BUNDLE ${ALL_SRC} src/main_LineViewerDemo.cpp  
                           ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS} ${RES_FILES} ${MANTID_RC_FILE} )
target_link_libraries ( LineViewerDemo 
                       ${CORE_MANTIDLIBS} MantidQtAPI MantidWidgets  MantidQtSliceViewer  MDEvents Geometry
                       QtPropertyBrowser ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${QWT_LIBRARIES} )
                       
endif ()

###########################################################################
# Unit tests setup
###########################################################################

# Check system version
if ( ${CMAKE_SYSTEM} MATCHES ".*\\.el5" )
  	# This is RHEL5. GUI tests hang on this platform for some reason
  	message ( "RHEL5 hangs on GUI tests. Disabling MantidQt SliceViewer python tests." )
else ()

	if ( PYUNITTEST_FOUND )
	  pyunittest_add_test ( SliceViewerMantidPlotTest.py ${TEST_PY_FILES} "" )
	  add_dependencies ( SliceViewerMantidPlotTest.py MantidQt Algorithms )
	  add_dependencies ( GUITests SliceViewerMantidPlotTest.py )
	  # Add to the 'UnitTests' group in VS
	  set_property ( TARGET SliceViewerMantidPlotTest.py PROPERTY FOLDER "UnitTests" )
	endif ()
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtSliceViewer DESTINATION ${LIB_DIR} )


