# ######################################################################################################################
# Matplotlib-based implementation
# ######################################################################################################################
set(MPL_SRC_FILES
    src/ContourPreviewPlot.cpp
    src/ExternalPlotter.cpp
    src/PeakPicker.cpp
    src/PreviewPlot.cpp
    src/RangeSelector.cpp
    src/SingleSelector.cpp
    src/PlotWidget/PlotModel.cpp
    src/PlotWidget/PlotPresenter.cpp
    src/PlotWidget/QtPlotView.cpp
)

set(MPL_MOC_FILES
    inc/MantidQtWidgets/Plotting/ContourPreviewPlot.h inc/MantidQtWidgets/Plotting/PeakPicker.h
    inc/MantidQtWidgets/Plotting/RangeSelector.h inc/MantidQtWidgets/Plotting/SingleSelector.h
    inc/MantidQtWidgets/Plotting/PreviewPlot.h inc/MantidQtWidgets/Plotting/PlotWidget/QtPlotView.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set(MPL_INC_FILES
    ${MPL_MOC_FILES}
    inc/MantidQtWidgets/Plotting/ExternalPlotter.h
    inc/MantidQtWidgets/Plotting/AxisID.h
    inc/MantidQtWidgets/Plotting/PlotWidget/IPlotView.h
    inc/MantidQtWidgets/Plotting/MockExternalPlotter.h
    inc/MantidQtWidgets/Plotting/PlotWidget/PlotModel.h
    inc/MantidQtWidgets/Plotting/PlotWidget/PlotPresenter.h
    inc/MantidQtWidgets/Plotting/PlotWidget/QtPlotView.h
)

mtd_add_qt_library(
  TARGET_NAME MantidQtWidgetsPlotting
  QT_VERSION 5
  SRC ${MPL_SRC_FILES}
  MOC ${MPL_MOC_FILES}
  NOMOC ${MPL_INC_FILES}
  DEFS IN_MANTIDQT_PLOTTING
  INCLUDE_DIRS inc
  LINK_LIBS ${CORE_MANTIDLIBS} Mantid::PythonInterfaceCore ${POCO_LIBRARIES} gmock
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsMplCpp
  INSTALL_DIR ${WORKBENCH_LIB_DIR}
  OSX_INSTALL_RPATH @loader_path/../MacOS @loader_path/../Frameworks
  LINUX_INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}"
)

set(TEST_FILES test/ContourPreviewPlotTest.h test/ExternalPlotterTest.h test/PlotWidget/PlotModelTest.h
               test/PlotWidget/PlotPresenterTest.h
)

set(CXXTEST_EXTRA_HEADER_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/test/PlottingTestInitialization.h")
# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  mtd_add_qt_tests(
    TARGET_NAME MantidQtWidgetsPlottingTest
    QT_VERSION 5
    INCLUDE_DIRS inc ../common/inc ../../../Framework/DataObjects/inc
    SRC ${TEST_FILES}
    LINK_LIBS ${CORE_MANTIDLIBS}
              Mantid::DataObjects
              ${POCO_LIBRARIES}
              ${Boost_LIBRARIES}
              Mantid::PythonInterfaceCore
              gmock
              Python::Python
    MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsPlotting MantidQtWidgetsMplCpp
    PARENT_DEPENDENCIES GUITests
  )

  add_framework_test_helpers(MantidQtWidgetsPlottingTestQt5)
endif()
