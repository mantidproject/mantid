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
    ScalarMappableTest.h)

set(CXXTEST_EXTRA_HEADER_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/MplCppTestInitialization.h)
mtd_add_qt_tests(TARGET_NAME MantidQtWidgetsMplCppTest
                 QT_VERSION 5
                 INCLUDE_DIRS
                   ${CMAKE_CURRENT_LIST_DIR}/../inc
                 SRC
                   ${TEST_FILES}
                 LINK_LIBS
                   API
                   DataObjects
                   HistogramData
                   Kernel
                   PythonInterfaceCore
                   Types
                   ${Boost_LIBRARIES}
                   ${POCO_LIBRARIES}
                   ${PYTHON_LIBRARIES}
                   ${POCO_LIBRARIES}
                 MTD_QT_LINK_LIBS
                   MantidQtWidgetsMplCpp
                   MantidQtWidgetsCommon
                 PARENT_DEPENDENCIES
                   GUITests)
