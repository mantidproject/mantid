# ######################################################################################################################
# Matplotlib-based implementation
# ######################################################################################################################
set(MPL_SRC_FILES src/Mpl/ContourPreviewPlot.cpp src/Mpl/ExternalPlotter.cpp src/Mpl/PeakPicker.cpp
                  src/Mpl/PreviewPlot.cpp src/Mpl/RangeSelector.cpp src/Mpl/SingleSelector.cpp
)

set(MPL_MOC_FILES
    inc/MantidQtWidgets/Plotting/Mpl/ContourPreviewPlot.h inc/MantidQtWidgets/Plotting/Mpl/ExternalPlotter.h
    inc/MantidQtWidgets/Plotting/Mpl/PeakPicker.h inc/MantidQtWidgets/Plotting/Mpl/PreviewPlot.h
    inc/MantidQtWidgets/Plotting/Mpl/RangeSelector.h inc/MantidQtWidgets/Plotting/Mpl/SingleSelector.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set(MPL_INC_FILES
    ${MPL_MOC_FILES} inc/MantidQtWidgets/Plotting/AxisID.h inc/MantidQtWidgets/Plotting/ContourPreviewPlot.h
    inc/MantidQtWidgets/Plotting/PeakPicker.h inc/MantidQtWidgets/Plotting/PreviewPlot.h
)

set(MPL_UI_FILES inc/MantidQtWidgets/Plotting/ContourPreviewPlot.ui inc/MantidQtWidgets/Plotting/PreviewPlot.ui)

mtd_add_qt_library(
  TARGET_NAME MantidQtWidgetsPlotting
  QT_VERSION 5
  SRC ${MPL_SRC_FILES}
  MOC ${MPL_MOC_FILES}
  NOMOC ${MPL_INC_FILES}
  UI ${MPL_UI_FILES}
  DEFS IN_MANTIDQT_PLOTTING
  INCLUDE_DIRS inc
  LINK_LIBS ${CORE_MANTIDLIBS} Mantid::PythonInterfaceCore ${POCO_LIBRARIES}
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsMplCpp
  INSTALL_DIR ${WORKBENCH_LIB_DIR}
  OSX_INSTALL_RPATH @loader_path/../MacOS @loader_path/../Frameworks
  LINUX_INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}"
)

set(TEST_FILES test/ContourPreviewPlotTest.h test/ExternalPlotterTest.h)

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
