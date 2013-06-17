set ( SRC_FILES
	src/ColorBarWidget.cpp
	src/CompositePeaksPresenter.cpp
	src/ConcretePeaksPresenter.cpp
	src/CustomTools.cpp
	src/DimensionSliceWidget.cpp
	src/LineOverlay.cpp
	src/LineViewer.cpp
	src/PeakBoundingBox.cpp
	src/PeaksViewerOptionsDialog.cpp
	src/PeakOverlayMultiCross.cpp
	src/PeakOverlayMultiCrossFactory.cpp
	src/PeakOverlayMultiSphere.cpp
	src/PeakOverlayMultiSphereFactory.cpp
	src/PeakOverlayViewFactoryBase.cpp
	src/PeakOverlayViewFactorySelector.cpp  
	src/PeakPalette.cpp
	src/PeakTransform.cpp
	src/PeakTransformHKL.cpp
	src/PeakTransformQSample.cpp
	src/PeakTransformQLab.cpp
	src/PeakTransformSelector.cpp
	src/PeaksViewer.cpp
	src/PeaksWorkspaceWidget.cpp
	src/PhysicalCrossPeak.cpp
	src/PhysicalSphericalPeak.cpp
	src/ProxyCompositePeaksPresenter.cpp
	src/QPeaksTableModel.cpp
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
	inc/MantidQtSliceViewer/CompositePeaksPresenter.h
	inc/MantidQtSliceViewer/ConcretePeaksPresenter.h
	inc/MantidQtSliceViewer/ConcretePeakTransformFactory.h
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/FirstExperimentInfoQuery.h
	inc/MantidQtSliceViewer/LineOverlay.h
	inc/MantidQtSliceViewer/LinePlotOptions.h
	inc/MantidQtSliceViewer/LineViewer.h
	inc/MantidQtSliceViewer/NullPeaksPresenter.h
	inc/MantidQtSliceViewer/PeakBoundingBox.h
	inc/MantidQtSliceViewer/PeakOverlayMultiCross.h
	inc/MantidQtSliceViewer/PeakOverlayMultiCrossFactory.h
	inc/MantidQtSliceViewer/PeakOverlayMultiSphere.h
	inc/MantidQtSliceViewer/PeakOverlayMultiSphereFactory.h
	inc/MantidQtSliceViewer/PeakOverlayView.h
	inc/MantidQtSliceViewer/PeakOverlayViewFactory.h
	inc/MantidQtSliceViewer/PeakOverlayViewFactoryBase.h
	inc/MantidQtSliceViewer/PeakOverlayViewFactorySelector.h
	inc/MantidQtSliceViewer/PeakPalette.h
	inc/MantidQtSliceViewer/PeaksPresenter.h
	inc/MantidQtSliceViewer/PeakTransformHKL.h
	inc/MantidQtSliceViewer/PeakTransformQSample.h
	inc/MantidQtSliceViewer/PeakTransformQLab.h
	inc/MantidQtSliceViewer/PeakTransform.h
	inc/MantidQtSliceViewer/PeakTransformFactory.h 
	inc/MantidQtSliceViewer/PeakTransformSelector.h
	inc/MantidQtSliceViewer/PeaksViewer.h
	inc/MantidQtSliceViewer/PeaksViewerOptionsDialog.h
	inc/MantidQtSliceViewer/PeaksWorkspaceWidget.h
	inc/MantidQtSliceViewer/PhysicalCrossPeak.h
	inc/MantidQtSliceViewer/PhysicalSphericalPeak.h
	inc/MantidQtSliceViewer/ProxyCompositePeaksPresenter.h
	inc/MantidQtSliceViewer/QPeaksTableModel.h
	inc/MantidQtSliceViewer/QScienceSpinBox.h
	inc/MantidQtSliceViewer/QwtRasterDataMD.h
	inc/MantidQtSliceViewer/SliceViewer.h
	inc/MantidQtSliceViewer/SliceViewerWindow.h
	inc/MantidQtSliceViewer/SnapToGridDialog.h
	inc/MantidQtSliceViewer/XYLimitsDialog.h
	inc/MantidQtSliceViewer/ZoomablePeaksView.h
)

set ( MOC_FILES 
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/LineOverlay.h
	inc/MantidQtSliceViewer/LineViewer.h
	inc/MantidQtSliceViewer/LinePlotOptions.h
 	inc/MantidQtSliceViewer/PeakOverlayMultiCross.h
	inc/MantidQtSliceViewer/PeakOverlayMultiSphere.h
	inc/MantidQtSliceViewer/SliceViewer.h
	inc/MantidQtSliceViewer/SliceViewerWindow.h
	inc/MantidQtSliceViewer/ColorBarWidget.h
	inc/MantidQtSliceViewer/QScienceSpinBox.h
	inc/MantidQtSliceViewer/SnapToGridDialog.h
	inc/MantidQtSliceViewer/PeaksViewer.h
	inc/MantidQtSliceViewer/PeaksViewerOptionsDialog.h
	inc/MantidQtSliceViewer/PeaksWorkspaceWidget.h
	inc/MantidQtSliceViewer/XYLimitsDialog.h
	inc/MantidQtSliceViewer/QPeaksTableModel.h
)

set ( UI_FILES 
	inc/MantidQtSliceViewer/DimensionSliceWidget.ui
	inc/MantidQtSliceViewer/SliceViewer.ui
	inc/MantidQtSliceViewer/LineViewer.ui
	inc/MantidQtSliceViewer/LinePlotOptions.ui
	inc/MantidQtSliceViewer/ColorBarWidget.ui
	inc/MantidQtSliceViewer/SnapToGridDialog.ui
	inc/MantidQtSliceViewer/PeaksWorkspaceWidget.ui
	inc/MantidQtSliceViewer/PeaksViewerOptionsDialog.ui
	inc/MantidQtSliceViewer/XYLimitsDialog.ui
)

set ( TEST_FILES
    test/CompositePeaksPresenterTest.h
    test/ConcretePeaksPresenterTest.h
    test/PeakBoundingBoxTest.h
    test/PeakPaletteTest.h
    test/PeakOverlayViewFactorySelectorTest.h
    test/PeakTransformHKLTest.h
    test/PeakTransformQSampleTest.h
    test/PeakTransformQLabTest.h
    test/PeakTransformSelectorTest.h
    test/PhysicalCrossPeakTest.h
    test/PhysicalSphericalPeakTest.h
    test/NullPeaksPresenterTest.h
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
  
  pyunittest_add_test (${CMAKE_CURRENT_SOURCE_DIR}/test SliceViewerMantidPlotTest ${TEST_PY_FILES} )
endif ()

if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories( ../../Framework/TestHelpers/inc ../../Framework/DataObjects/inc ../../Framework/Crystal/inc )

  set ( TESTHELPER_SRCS ../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp 
  ../../Framework/TestHelpers/src/ComponentCreationHelper.cpp 
  )

  cxxtest_add_test ( SliceViewerTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
  target_link_libraries( SliceViewerTest MantidQtSliceViewer DataObjects ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )

  add_dependencies( GUITests SliceViewerTest )
  # Add to the 'UnitTests' group in VS
  set_property( TARGET SliceViewerTest PROPERTY FOLDER "UnitTests" )

endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtSliceViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


