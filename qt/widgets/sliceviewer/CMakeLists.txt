set ( SRC_FILES
	src/CompositePeaksPresenter.cpp
	src/ConcretePeaksPresenter.cpp
	src/CoordinateTransform.cpp
	src/CustomTools.cpp
	src/DimensionSliceWidget.cpp
	src/EllipsoidPlaneSliceCalculator.cpp
	src/LineOverlay.cpp
	src/LineViewer.cpp
	src/PeakBoundingBox.cpp
	src/PeaksViewerOverlayDialog.cpp
        src/PeakOverlayInteractive.cpp
	src/PeakOverlayViewFactoryBase.cpp
	src/PeakPalette.cpp
	src/PeakRepresentation.cpp
	src/PeakRepresentationCross.cpp
        src/PeakRepresentationEllipsoid.cpp
        src/PeakRepresentationSphere.cpp
	src/PeaksTableColumnsDialog.cpp
	src/PeakView.cpp
	src/PeakViewFactory.cpp
	src/PeaksViewer.cpp
	src/PeaksWorkspaceWidget.cpp
	src/ProxyCompositePeaksPresenter.cpp
	src/QPeaksTableModel.cpp
	src/SliceViewer.cpp
	src/SliceViewerFunctions.cpp
	src/SliceViewerWindow.cpp
	src/SnapToGridDialog.cpp
	src/XYLimitsDialog.cpp
	src/LinePlotOptions.cpp
	src/NonOrthogonalOverlay.cpp
	src/QwtScaleDrawNonOrthogonal.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	inc/MantidQtSliceViewer/CompositePeaksPresenter.h
	inc/MantidQtSliceViewer/ConcretePeaksPresenter.h
	inc/MantidQtSliceViewer/CoordinateTransform.h
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/EllipsoidPlaneSliceCalculator.h
	inc/MantidQtSliceViewer/FirstExperimentInfoQuery.h
	inc/MantidQtSliceViewer/LineOverlay.h
	inc/MantidQtSliceViewer/LinePlotOptions.h
	inc/MantidQtSliceViewer/LineViewer.h
	inc/MantidQtSliceViewer/NullPeaksPresenter.h
	inc/MantidQtSliceViewer/NonOrthogonalOverlay.h
	inc/MantidQtSliceViewer/PeakEditMode.h
        inc/MantidQtSliceViewer/PeakBoundingBox.h
        inc/MantidQtSliceViewer/PeakOverlayInteractive.h
	inc/MantidQtSliceViewer/PeakOverlayView.h
	inc/MantidQtSliceViewer/PeakOverlayViewFactory.h
	inc/MantidQtSliceViewer/PeakOverlayViewFactoryBase.h
	inc/MantidQtSliceViewer/PeakPalette.h
	inc/MantidQtSliceViewer/PeaksPresenter.h
	inc/MantidQtSliceViewer/PeakPrimitives.h
        inc/MantidQtSliceViewer/PeakRepresentation.h
	inc/MantidQtSliceViewer/PeakRepresentationCross.h
        inc/MantidQtSliceViewer/PeakRepresentationEllipsoid.h
        inc/MantidQtSliceViewer/PeakRepresentationSphere.h
	inc/MantidQtSliceViewer/PeaksTableColumnsDialog.h
	inc/MantidQtSliceViewer/PeakView.h
	inc/MantidQtSliceViewer/PeakViewColor.h
	inc/MantidQtSliceViewer/PeakViewFactory.h
	inc/MantidQtSliceViewer/PeaksViewer.h
	inc/MantidQtSliceViewer/PeaksViewerOverlayDialog.h
	inc/MantidQtSliceViewer/PeaksWorkspaceWidget.h
	inc/MantidQtSliceViewer/ProxyCompositePeaksPresenter.h
	inc/MantidQtSliceViewer/QPeaksTableModel.h
	inc/MantidQtSliceViewer/SliceViewer.h
	inc/MantidQtSliceViewer/SliceViewerFunctions.h
	inc/MantidQtSliceViewer/SliceViewerWindow.h
	inc/MantidQtSliceViewer/SnapToGridDialog.h
	inc/MantidQtSliceViewer/UpdateableOnDemand.h
	inc/MantidQtSliceViewer/XYLimitsDialog.h
	inc/MantidQtSliceViewer/ZoomableOnDemand.h
	inc/MantidQtSliceViewer/ZoomablePeaksView.h
	inc/MantidQtSliceViewer/QwtScaleDrawNonOrthogonal.h
)

set ( MOC_FILES
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/LineOverlay.h
	inc/MantidQtSliceViewer/LineViewer.h
	inc/MantidQtSliceViewer/LinePlotOptions.h
	inc/MantidQtSliceViewer/NonOrthogonalOverlay.h
        inc/MantidQtSliceViewer/PeakOverlayInteractive.h
	inc/MantidQtSliceViewer/PeaksTableColumnsDialog.h
	inc/MantidQtSliceViewer/SliceViewer.h
	inc/MantidQtSliceViewer/SliceViewerWindow.h
	inc/MantidQtSliceViewer/SnapToGridDialog.h
	inc/MantidQtSliceViewer/PeaksViewer.h
	inc/MantidQtSliceViewer/PeaksViewerOverlayDialog.h
	inc/MantidQtSliceViewer/PeaksWorkspaceWidget.h
	inc/MantidQtSliceViewer/XYLimitsDialog.h
	inc/MantidQtSliceViewer/QPeaksTableModel.h
)

set ( UI_FILES
        inc/MantidQtSliceViewer/DimensionSliceWidget.ui
	inc/MantidQtSliceViewer/SliceViewer.ui
	inc/MantidQtSliceViewer/LineViewer.ui
	inc/MantidQtSliceViewer/LinePlotOptions.ui
	inc/MantidQtSliceViewer/SnapToGridDialog.ui
	inc/MantidQtSliceViewer/PeaksTableColumnsDialog.ui
	inc/MantidQtSliceViewer/PeaksWorkspaceWidget.ui
	inc/MantidQtSliceViewer/PeaksViewerOverlayDialog.ui
	inc/MantidQtSliceViewer/XYLimitsDialog.ui
)

set ( TEST_FILES
    test/CompositePeaksPresenterTest.h
    test/CoordinateTransformTest.h
    test/ConcretePeaksPresenterTest.h
    test/EllipsoidPlaneSliceCalculatorTest.h
    test/PeakBoundingBoxTest.h
    test/PeakPaletteTest.h
    test/PeakRepresentationCrossTest.h
    test/PeakRepresentationSphereTest.h
    test/PeakRepresentationEllipsoidTest.h
    test/NullPeaksPresenterTest.h
    test/SliceViewerFunctionsTest.h
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

include_directories ( ${CMAKE_CURRENT_BINARY_DIR})

include_directories ( ../../Framework/DataObjects/inc )

# For Windows:
add_definitions ( -DIN_MANTIDQT_SLICEVIEWER )

# This creates the target library
add_library ( MantidQtSliceViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtSliceViewer PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

# ... and links to other required libs
target_link_libraries ( MantidQtSliceViewer LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtAPI
            MantidWidgets
            DataObjects
            ${CORE_MANTIDLIBS}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES} )

###########################################################################
# Unit tests setup
###########################################################################

if ( PYUNITTEST_FOUND )
  pyunittest_add_test (${CMAKE_CURRENT_SOURCE_DIR}/test python.test ${TEST_PY_FILES} )
endif ()

if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories( ../../Framework/TestHelpers/inc ../../Framework/DataObjects/inc ../../Framework/Crystal/inc)

  set ( TESTHELPER_SRCS
    ../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
  )

  cxxtest_add_test ( SliceViewerTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
  target_link_libraries( SliceViewerTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtSliceViewer
            DataObjects
            Crystal
            ${CORE_MANTIDLIBS}
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            ${QT_LIBRARIES}
            ${GMOCK_LIBRARIES}
            ${GTEST_LIBRARIES} )

  add_dependencies( GUITests SliceViewerTest )
  # Add to the 'UnitTests' group in VS
  set_property( TARGET SliceViewerTest PROPERTY FOLDER "UnitTests" )

endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtSliceViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
