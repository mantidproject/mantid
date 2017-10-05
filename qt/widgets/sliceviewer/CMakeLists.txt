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
	inc/MantidQtWidgets/SliceViewer/CompositePeaksPresenter.h
	inc/MantidQtWidgets/SliceViewer/ConcretePeaksPresenter.h
	inc/MantidQtWidgets/SliceViewer/CoordinateTransform.h
	inc/MantidQtWidgets/SliceViewer/CustomTools.h
	inc/MantidQtWidgets/SliceViewer/DimensionSliceWidget.h
	inc/MantidQtWidgets/SliceViewer/EllipsoidPlaneSliceCalculator.h
	inc/MantidQtWidgets/SliceViewer/FirstExperimentInfoQuery.h
	inc/MantidQtWidgets/SliceViewer/LineOverlay.h
	inc/MantidQtWidgets/SliceViewer/LinePlotOptions.h
	inc/MantidQtWidgets/SliceViewer/LineViewer.h
	inc/MantidQtWidgets/SliceViewer/NullPeaksPresenter.h
	inc/MantidQtWidgets/SliceViewer/NonOrthogonalOverlay.h
	inc/MantidQtWidgets/SliceViewer/PeakEditMode.h
    inc/MantidQtWidgets/SliceViewer/PeakBoundingBox.h
    inc/MantidQtWidgets/SliceViewer/PeakOverlayInteractive.h
	inc/MantidQtWidgets/SliceViewer/PeakOverlayView.h
	inc/MantidQtWidgets/SliceViewer/PeakOverlayViewFactory.h
	inc/MantidQtWidgets/SliceViewer/PeakOverlayViewFactoryBase.h
	inc/MantidQtWidgets/SliceViewer/PeakPalette.h
	inc/MantidQtWidgets/SliceViewer/PeaksPresenter.h
	inc/MantidQtWidgets/SliceViewer/PeakPrimitives.h
    inc/MantidQtWidgets/SliceViewer/PeakRepresentation.h
	inc/MantidQtWidgets/SliceViewer/PeakRepresentationCross.h
    inc/MantidQtWidgets/SliceViewer/PeakRepresentationEllipsoid.h
    inc/MantidQtWidgets/SliceViewer/PeakRepresentationSphere.h
	inc/MantidQtWidgets/SliceViewer/PeaksTableColumnsDialog.h
	inc/MantidQtWidgets/SliceViewer/PeakView.h
	inc/MantidQtWidgets/SliceViewer/PeakViewColor.h
	inc/MantidQtWidgets/SliceViewer/PeakViewFactory.h
	inc/MantidQtWidgets/SliceViewer/PeaksViewer.h
	inc/MantidQtWidgets/SliceViewer/PeaksViewerOverlayDialog.h
	inc/MantidQtWidgets/SliceViewer/PeaksWorkspaceWidget.h
	inc/MantidQtWidgets/SliceViewer/ProxyCompositePeaksPresenter.h
	inc/MantidQtWidgets/SliceViewer/QPeaksTableModel.h
	inc/MantidQtWidgets/SliceViewer/SliceViewer.h
	inc/MantidQtWidgets/SliceViewer/SliceViewerFunctions.h
	inc/MantidQtWidgets/SliceViewer/SliceViewerWindow.h
	inc/MantidQtWidgets/SliceViewer/SnapToGridDialog.h
	inc/MantidQtWidgets/SliceViewer/UpdateableOnDemand.h
	inc/MantidQtWidgets/SliceViewer/XYLimitsDialog.h
	inc/MantidQtWidgets/SliceViewer/ZoomableOnDemand.h
	inc/MantidQtWidgets/SliceViewer/ZoomablePeaksView.h
	inc/MantidQtWidgets/SliceViewer/QwtScaleDrawNonOrthogonal.h
)

set ( MOC_FILES
	inc/MantidQtWidgets/SliceViewer/CustomTools.h
	inc/MantidQtWidgets/SliceViewer/DimensionSliceWidget.h
	inc/MantidQtWidgets/SliceViewer/LineOverlay.h
	inc/MantidQtWidgets/SliceViewer/LineViewer.h
	inc/MantidQtWidgets/SliceViewer/LinePlotOptions.h
	inc/MantidQtWidgets/SliceViewer/NonOrthogonalOverlay.h
    inc/MantidQtWidgets/SliceViewer/PeakOverlayInteractive.h
	inc/MantidQtWidgets/SliceViewer/PeaksTableColumnsDialog.h
	inc/MantidQtWidgets/SliceViewer/SliceViewer.h
	inc/MantidQtWidgets/SliceViewer/SliceViewerWindow.h
	inc/MantidQtWidgets/SliceViewer/SnapToGridDialog.h
	inc/MantidQtWidgets/SliceViewer/PeaksViewer.h
	inc/MantidQtWidgets/SliceViewer/PeaksViewerOverlayDialog.h
	inc/MantidQtWidgets/SliceViewer/PeaksWorkspaceWidget.h
	inc/MantidQtWidgets/SliceViewer/XYLimitsDialog.h
	inc/MantidQtWidgets/SliceViewer/QPeaksTableModel.h
)

set ( UI_FILES
    inc/MantidQtWidgets/SliceViewer/DimensionSliceWidget.ui
	inc/MantidQtWidgets/SliceViewer/SliceViewer.ui
	inc/MantidQtWidgets/SliceViewer/LineViewer.ui
	inc/MantidQtWidgets/SliceViewer/LinePlotOptions.ui
	inc/MantidQtWidgets/SliceViewer/SnapToGridDialog.ui
	inc/MantidQtWidgets/SliceViewer/PeaksTableColumnsDialog.ui
	inc/MantidQtWidgets/SliceViewer/PeaksWorkspaceWidget.ui
	inc/MantidQtWidgets/SliceViewer/PeaksViewerOverlayDialog.ui
	inc/MantidQtWidgets/SliceViewer/XYLimitsDialog.ui
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
include_directories ( ../../Framework/DataObjects/inc )

###########################################################################
# Main Library Target
###########################################################################
mtd_add_qt_library (TARGET_NAME MantidQtWidgetsSliceViewer
                    SRC ${SRC_FILES}
                    MOC ${MOC_FILES}
                    NOMOC ${INC_FILES}
                    UI ${UI_FILES}
                    RES ../../resources/icons/SliceViewerIcons.qrc
                    DEFS IN_MANTIDQT_SLICEVIEWER
                    LINK_LIBS
                        DataObjects
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${CORE_MANTIDLIBS}
                        ${POCO_LIBRARIES}
                        ${Boost_LIBRARIES}
                        ${QT_LIBRARIES}
                        ${QWT_LIBRARIES}
                    MTD_QT_LINK_LIBS
                        MantidQtWidgetsCommon
)

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtWidgetsSliceViewerQt4 PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

###########################################################################
# Unit tests setup
###########################################################################

if ( PYUNITTEST_FOUND )
  pyunittest_add_test (${CMAKE_CURRENT_SOURCE_DIR}/test python.test ${TEST_PY_FILES} )
endif ()

if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories( ../../../Framework/TestHelpers/inc ../../../Framework/DataObjects/inc ../../../Framework/Crystal/inc)

  set ( TESTHELPER_SRCS
    ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
  )

  cxxtest_add_test ( SliceViewerTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
  target_link_libraries( SliceViewerTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsSliceViewerQt4
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
