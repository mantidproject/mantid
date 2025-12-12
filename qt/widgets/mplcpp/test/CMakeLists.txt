# Unit tests for MplCpp library
set(TEST_FILES
    ArtistTest.h
    AxesTest.h
    ColormapTest.h
    ColorConverterTest.h
    ColorsTest.h
    CyclerTest.h
    FigureTest.h
    FigureCanvasQtTest.h
    Line2DTest.h
    MantidAxesTest.h
    MantidColormapTest.h
    PanZoomToolTest.h
    PlotTest.h
    QBackendExtractTest.h
    ScalarMappableTest.h
)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/MplCppTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtWidgetsMplCppTest
  QT_VERSION 5
  INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../inc
  SRC ${TEST_FILES}
  LINK_LIBS Mantid::API
            Mantid::DataObjects
            Mantid::HistogramData
            Mantid::Kernel
            Mantid::PythonInterfaceCore
            Mantid::Types
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            Python::Python
  MTD_QT_LINK_LIBS MantidQtWidgetsMplCpp MantidQtWidgetsCommon
  PARENT_DEPENDENCIES GUITests
)

if(TARGET GTest::gmock AND TARGET GTest::gtest)

  target_link_libraries(MantidQtWidgetsMplCppTestQt5 PRIVATE GTest::gmock GTest::gtest)
else()
  target_link_libraries(MantidQtWidgetsMplCppTestQt5 PRIVATE gmock gtest)
endif()
