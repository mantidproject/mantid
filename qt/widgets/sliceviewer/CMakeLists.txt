set(SRC_FILES
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
    src/QwtScaleDrawNonOrthogonal.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
set(INC_FILES
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
    inc/MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h
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
    inc/MantidQtWidgets/SliceViewer/QwtScaleDrawNonOrthogonal.h)

set(MOC_FILES
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
    inc/MantidQtWidgets/SliceViewer/QPeaksTableModel.h)

set(UI_FILES
    inc/MantidQtWidgets/SliceViewer/DimensionSliceWidget.ui
    inc/MantidQtWidgets/SliceViewer/SliceViewer.ui
    inc/MantidQtWidgets/SliceViewer/LineViewer.ui
    inc/MantidQtWidgets/SliceViewer/LinePlotOptions.ui
    inc/MantidQtWidgets/SliceViewer/SnapToGridDialog.ui
    inc/MantidQtWidgets/SliceViewer/PeaksTableColumnsDialog.ui
    inc/MantidQtWidgets/SliceViewer/PeaksWorkspaceWidget.ui
    inc/MantidQtWidgets/SliceViewer/PeaksViewerOverlayDialog.ui
    inc/MantidQtWidgets/SliceViewer/XYLimitsDialog.ui)

#
# Main Library Target
#
mtd_add_qt_library(TARGET_NAME MantidQtWidgetsSliceViewer
                   QT_VERSION 4
                   SRC ${SRC_FILES}
                   MOC ${MOC_FILES}
                   NOMOC ${INC_FILES}
                   UI ${UI_FILES}
                   RES ../../resources/icons/SliceViewerIcons.qrc
                   DEFS
                     IN_MANTIDQT_SLICEVIEWER
                   INCLUDE_DIRS
                     inc
                     ../../../Framework/DataObjects/inc
                     ../../../Framework/Crystal/inc
                   LINK_LIBS
                     DataObjects
                     ${TCMALLOC_LIBRARIES_LINKTIME}
                     ${CORE_MANTIDLIBS}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                   QT4_LINK_LIBS
                     Qwt5
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsLegacyQwt
                   INSTALL_DIR
                     ${LIB_DIR}
                   OSX_INSTALL_RPATH
                     @loader_path/../MacOS
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../${LIB_DIR}")

#
# Testing
#

set(TEST_FILES
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
    test/SliceViewerFunctionsTest.h)

mtd_add_qt_tests(TARGET_NAME MantidQtWidgetsSliceViewerTest
                 QT_VERSION 4
                 SRC ${TEST_FILES}
                 INCLUDE_DIRS
                   ../../../Framework/TestHelpers/inc
                   ../../../Framework/DataObjects/inc
                   ../../../Framework/Crystal/inc
                 TEST_HELPER_SRCS
                   ../../../Framework/TestHelpers/src/TearDownWorld.cpp
                   ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                   ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
                   ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                 LINK_LIBS
                   ${TCMALLOC_LIBRARIES_LINKTIME}
                   ${CORE_MANTIDLIBS}
                   Crystal
                   DataObjects
                   ${GMOCK_LIBRARIES}
                   ${GTEST_LIBRARIES}
                   ${POCO_LIBRARIES}
                   ${Boost_LIBRARIES}
                 QT4_LINK_LIBS
                   Qwt5
                 MTD_QT_LINK_LIBS
                   MantidQtWidgetsSliceViewer
                 PARENT_DEPENDENCIES
                   GUITests)

# Python unit tests
set(TEST_PY_FILES
	test/SliceViewerPythonInterfaceTest.py
    test/QwtTypesPythonInterfaceTest.py)

if(PYUNITTEST_FOUND)
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.test ${TEST_PY_FILES})
endif()
