set(SRC_FILES
    src/ContourPreviewPlot.cpp
    src/DraggableColorBarWidget.cpp
    src/MantidColorMap.cpp
    src/MantidQwtIMDWorkspaceData.cpp
    src/MantidQwtWorkspaceData.cpp
    src/MWView.cpp
    src/PowerScaleEngine.cpp
    src/QwtHelper.cpp
    src/QwtRasterDataMD.cpp
    src/QwtRasterDataMDNonOrthogonal.cpp
    src/QwtWorkspaceBinData.cpp
    src/QwtWorkspaceSpectrumData.cpp
    src/ScaleEngine.cpp
    src/ColorBarWidget.cpp
    src/DisplayCurveFit.cpp
    src/ErrorCurve.cpp
    src/PeakPicker.cpp
    src/PreviewPlot.cpp
    src/RangeSelector.cpp
    src/SafeQwtPlot.cpp
    src/SignalRange.cpp)

set(MOC_FILES
    inc/MantidQtWidgets/LegacyQwt/ContourPreviewPlot.h
    inc/MantidQtWidgets/LegacyQwt/DraggableColorBarWidget.h
    inc/MantidQtWidgets/LegacyQwt/ColorBarWidget.h
    inc/MantidQtWidgets/LegacyQwt/MWView.h
    inc/MantidQtWidgets/LegacyQwt/RangeSelector.h
    inc/MantidQtWidgets/LegacyQwt/PeakPicker.h
    inc/MantidQtWidgets/LegacyQwt/DisplayCurveFit.h
    inc/MantidQtWidgets/LegacyQwt/PreviewPlot.h
    inc/MantidQtWidgets/LegacyQwt/SafeQwtPlot.h)

# Include files aren't required, but this makes them appear in Visual Studio
set(INC_FILES
    inc/MantidQtWidgets/LegacyQwt/ContourPreviewPlot.h
    inc/MantidQtWidgets/LegacyQwt/MantidQwtWorkspaceData.h
    inc/MantidQtWidgets/LegacyQwt/QwtRasterDataMD.h
    inc/MantidQtWidgets/LegacyQwt/QwtRasterDataMDNonOrthogonal.h
    inc/MantidQtWidgets/LegacyQwt/QwtWorkspaceBinData.h
    inc/MantidQtWidgets/LegacyQwt/PowerScaleEngine.h
    inc/MantidQtWidgets/LegacyQwt/ScaleEngine.h
    inc/MantidQtWidgets/LegacyQwt/MantidQwtIMDWorkspaceData.h
    inc/MantidQtWidgets/LegacyQwt/SafeQwtPlot.h
    inc/MantidQtWidgets/LegacyQwt/MantidColorMap.h
    inc/MantidQtWidgets/LegacyQwt/ErrorCurve.h
    inc/MantidQtWidgets/LegacyQwt/QwtWorkspaceSpectrumData.h
    inc/MantidQtWidgets/LegacyQwt/QwtHelper.h
    inc/MantidQtWidgets/LegacyQwt/qwt_compat.h
    inc/MantidQtWidgets/LegacyQwt/SignalRange.h)

set(UI_FILES
    inc/MantidQtWidgets/LegacyQwt/ColorBarWidget.ui
    inc/MantidQtWidgets/LegacyQwt/ContourPreviewPlot.ui
    inc/MantidQtWidgets/LegacyQwt/DisplayCurveFit.ui
    inc/MantidQtWidgets/LegacyQwt/MWView.ui
    inc/MantidQtWidgets/LegacyQwt/PreviewPlot.ui)

# Target
mtd_add_qt_library(TARGET_NAME MantidQtWidgetsLegacyQwt
                   QT_VERSION 4
                   SRC ${SRC_FILES}
                   MOC ${MOC_FILES}
                   NOMOC ${INC_FILES}
                   UI ${UI_FILES}
                   DEFS
                     IN_MANTIDQT_LEGACYQWT
                   INCLUDE_DIRS
                     inc
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
                     "\$ORIGIN/../${LIB_DIR}")

# Testing
set(TEST_FILES
    test/ContourPreviewPlotTest.h
    test/MantidColorMapTest.h
    test/QwtWorkspaceBinDataTest.h
    test/QwtWorkspaceSpectrumDataTest.h)

mtd_add_qt_tests(TARGET_NAME MantidQtWidgetsLegacyQwtTest
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
                   MantidQtWidgetsLegacyQwt
                 PARENT_DEPENDENCIES
                   GUITests)
