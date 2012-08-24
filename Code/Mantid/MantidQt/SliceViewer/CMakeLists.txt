set ( SRC_FILES
	src/ColorBarWidget.cpp
	src/CustomTools.cpp
	src/DimensionSliceWidget.cpp
	src/LineOverlay.cpp
	src/LineViewer.cpp
    src/PeakOverlay.cpp
    src/PeakOverlayFactory.cpp
    src/PeaksPresenter.cpp
	src/QScienceSpinBox.cpp
	src/QwtRasterDataMD.cpp
	src/SliceViewer.cpp
	src/SliceViewerWindow.cpp
	src/SnapToGridDialog.cpp
	src/XYLimitsDialog.cpp
	src/LinePlotOptions.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	inc/MantidQtSliceViewer/ColorBarWidget.h
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/LineOverlay.h
	inc/MantidQtSliceViewer/LinePlotOptions.h
	inc/MantidQtSliceViewer/LineViewer.h
    inc/MantidQtSliceViewer/PeakOverlay.h
    inc/MantidQtSliceViewer/PeakOverlayView.h
    inc/MantidQtSliceViewer/PeakOverlayViewFactory.h
    inc/MantidQtSliceViewer/PeakOverlayFactory.h
    inc/MantidQtSliceViewer/PeaksPresenter.h
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
	inc/MantidQtSliceViewer/LinePlotOptions.h
    inc/MantidQtSliceViewer/PeakOverlay.h
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
	inc/MantidQtSliceViewer/LinePlotOptions.ui
	inc/MantidQtSliceViewer/ColorBarWidget.ui
	inc/MantidQtSliceViewer/SnapToGridDialog.ui
	inc/MantidQtSliceViewer/XYLimitsDialog.ui
)

set ( TEST_FILES
    test/PeaksPresenterTest.h
)

# Python unit tests
set ( TEST_PY_FILES 
	test/SliceViewerPythonInterfaceTest.py
	test/QwtTypesPythonInterfaceTest.py
)

include_directories ( inc )

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
target_link_libraries ( MantidQtSliceViewer MantidQtAPI MantidWidgets ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Unit tests setup
###########################################################################

if ( PYUNITTEST_FOUND )
	pyunittest_add_test ( SliceViewerMantidPlotTest.py ${TEST_PY_FILES} "" )
	add_dependencies ( SliceViewerMantidPlotTest.py MantidQtAPI MantidWidgets CustomDialogs 
	                   CustomInterfaces MantidQtSliceViewer Algorithms )
	add_dependencies ( GUITests SliceViewerMantidPlotTest.py )
	# Add to the 'UnitTests' group in VS
	set_property ( TARGET SliceViewerMantidPlotTest.py PROPERTY FOLDER "UnitTests" )
endif ()

if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories( ../../Framework/TestHelpers/inc ../../Framework/DataObjects/inc ../../Framework/Crystal/inc )

  set ( TESTHELPER_SRCS ../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp 
  ../../Framework/TestHelpers/src/ComponentCreationHelper.cpp   
  )

  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( SliceViewerTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( SliceViewerTest MantidQtSliceViewer DataObjects ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )

    add_dependencies( GUITests SliceViewerTest )
    # Add to the 'UnitTests' group in VS
    set_property( TARGET SliceViewerTest PROPERTY FOLDER "UnitTests" )
  endif ()

endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtSliceViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


