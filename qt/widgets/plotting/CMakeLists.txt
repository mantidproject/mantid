# Plotting support library for
#  - Qt4 this is Qwt based and
#  - Qt5 this is matplotlib based

###########################################################################
# Qwt5-based implementation
###########################################################################
set ( QWT_SRC_FILES
  src/Qwt/ContourPreviewPlot.cpp
  src/Qwt/DraggableColorBarWidget.cpp
  src/Qwt/MantidColorMap.cpp
  src/Qwt/MantidQwtIMDWorkspaceData.cpp
  src/Qwt/MantidQwtWorkspaceData.cpp
  src/Qwt/MWView.cpp
  src/Qwt/PowerScaleEngine.cpp
  src/Qwt/QwtHelper.cpp
  src/Qwt/QwtRasterDataMD.cpp
  src/Qwt/QwtRasterDataMDNonOrthogonal.cpp
  src/Qwt/QwtWorkspaceBinData.cpp
  src/Qwt/QwtWorkspaceSpectrumData.cpp
  src/Qwt/ScaleEngine.cpp
  src/Qwt/ColorBarWidget.cpp
  src/Qwt/DisplayCurveFit.cpp
  src/Qwt/ErrorCurve.cpp
  src/Qwt/PeakPicker.cpp
  src/Qwt/PreviewPlot.cpp
  src/Qwt/RangeSelector.cpp
  src/Qwt/SafeQwtPlot.cpp
  src/Qwt/SignalRange.cpp
  src/Qwt/SingleSelector.cpp
)

set ( QWT_MOC_FILES
  inc/MantidQtWidgets/Plotting/Qwt/ContourPreviewPlot.h
  inc/MantidQtWidgets/Plotting/Qwt/DraggableColorBarWidget.h
  inc/MantidQtWidgets/Plotting/Qwt/ColorBarWidget.h
  inc/MantidQtWidgets/Plotting/Qwt/MWView.h
  inc/MantidQtWidgets/Plotting/Qwt/RangeSelector.h
  inc/MantidQtWidgets/Plotting/Qwt/PeakPicker.h
  inc/MantidQtWidgets/Plotting/Qwt/DisplayCurveFit.h
  inc/MantidQtWidgets/Plotting/Qwt/PreviewPlot.h
  inc/MantidQtWidgets/Plotting/Qwt/SafeQwtPlot.h
  inc/MantidQtWidgets/Plotting/Qwt/SingleSelector.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( QWT_INC_FILES
  inc/MantidQtWidgets/Plotting/AxisID.h
  inc/MantidQtWidgets/Plotting/DllOption.h
  inc/MantidQtWidgets/Plotting/Qwt/PreviewPlot.h
  inc/MantidQtWidgets/Plotting/Qwt/MantidQwtWorkspaceData.h
  inc/MantidQtWidgets/Plotting/Qwt/QwtRasterDataMD.h
  inc/MantidQtWidgets/Plotting/Qwt/QwtRasterDataMDNonOrthogonal.h
  inc/MantidQtWidgets/Plotting/Qwt/QwtWorkspaceBinData.h
  inc/MantidQtWidgets/Plotting/Qwt/PowerScaleEngine.h
  inc/MantidQtWidgets/Plotting/Qwt/ScaleEngine.h
  inc/MantidQtWidgets/Plotting/Qwt/MantidQwtIMDWorkspaceData.h
  inc/MantidQtWidgets/Plotting/Qwt/SafeQwtPlot.h
  inc/MantidQtWidgets/Plotting/Qwt/MantidColorMap.h
  inc/MantidQtWidgets/Plotting/Qwt/ErrorCurve.h
  inc/MantidQtWidgets/Plotting/Qwt/QwtWorkspaceSpectrumData.h
  inc/MantidQtWidgets/Plotting/Qwt/QwtHelper.h
  inc/MantidQtWidgets/Plotting/Qwt/qwt_compat.h
  inc/MantidQtWidgets/Plotting/Qwt/SignalRange.h
)

set ( QWT_UI_FILES
  inc/MantidQtWidgets/Plotting/ContourPreviewPlot.ui
  inc/MantidQtWidgets/Plotting/Qwt/ColorBarWidget.ui
  inc/MantidQtWidgets/Plotting/Qwt/DisplayCurveFit.ui
  inc/MantidQtWidgets/Plotting/Qwt/MWView.ui
  inc/MantidQtWidgets/Plotting/PreviewPlot.ui
)

mtd_add_qt_library (TARGET_NAME MantidQtWidgetsPlotting
  QT_VERSION 4
  SRC ${QWT_SRC_FILES}
  MOC ${QWT_MOC_FILES}
  NOMOC ${QWT_INC_FILES}
  UI ${QWT_UI_FILES}
  DEFS
    IN_MANTIDQT_PLOTTING
  INCLUDE_DIRS
    inc
  SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
  QT4_LINK_LIBS
    Qwt5
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
  INSTALL_DIR
    ${LIB_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../MacOS
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../${LIB_DIR}"
)

set( TEST_FILES
  test/ContourPreviewPlotTest.h
  test/MantidColorMapTest.h
  test/QwtWorkspaceBinDataTest.h
  test/QwtWorkspaceSpectrumDataTest.h
  test/SignalRangeTest.h
)

mtd_add_qt_tests (TARGET_NAME MantidQtWidgetsPlottingTest
  QT_VERSION 4
  SRC ${TEST_FILES}
  INCLUDE_DIRS
    ../../../Framework/DataObjects/inc
    ../../../Framework/TestHelpers/inc
  TEST_HELPER_SRCS
    ../../../Framework/TestHelpers/src/TearDownWorld.cpp
    ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
  LINK_LIBS
    ${CORE_MANTIDLIBS}
    DataObjects
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES}
  QT4_LINK_LIBS
    Qwt5
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsPlotting
  PARENT_DEPENDENCIES
    GUITests
)

###########################################################################
# Matplotlib-based implementation
###########################################################################
set(MPL_SRC_FILES
    src/Mpl/ContourPreviewPlot.cpp
    src/Mpl/PeakPicker.cpp
    src/Mpl/PreviewPlot.cpp
    src/Mpl/RangeSelector.cpp
    src/Mpl/SingleSelector.cpp
)

set(MPL_MOC_FILES
    inc/MantidQtWidgets/Plotting/Mpl/ContourPreviewPlot.h
    inc/MantidQtWidgets/Plotting/Mpl/PeakPicker.h
    inc/MantidQtWidgets/Plotting/Mpl/PreviewPlot.h
    inc/MantidQtWidgets/Plotting/Mpl/RangeSelector.h
    inc/MantidQtWidgets/Plotting/Mpl/SingleSelector.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set(MPL_INC_FILES
    ${MPL_MOC_FILES}
    inc/MantidQtWidgets/Plotting/AxisID.h
    inc/MantidQtWidgets/Plotting/ContourPreviewPlot.h
    inc/MantidQtWidgets/Plotting/PeakPicker.h
    inc/MantidQtWidgets/Plotting/PreviewPlot.h
)

set(MPL_UI_FILES
    inc/MantidQtWidgets/Plotting/ContourPreviewPlot.ui
    inc/MantidQtWidgets/Plotting/PreviewPlot.ui
)

find_package(BoostPython)

mtd_add_qt_library (TARGET_NAME MantidQtWidgetsPlotting
  QT_VERSION 5
  SRC ${MPL_SRC_FILES}
  MOC ${MPL_MOC_FILES}
  NOMOC ${MPL_INC_FILES}
  UI ${MPL_UI_FILES}
  DEFS
    IN_MANTIDQT_PLOTTING
  INCLUDE_DIRS
    inc
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    PythonInterfaceCore
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsMplCpp
  INSTALL_DIR
    ${LIB_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../MacOS
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../${LIB_DIR}"
)
