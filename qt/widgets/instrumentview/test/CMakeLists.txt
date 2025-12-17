if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

# Testing
set(TEST_FILES
    InstrumentRendererTest.h
    InstrumentWidgetDecoderTest.h
    InstrumentWidgetEncoderTest.h
    InstrumentWidget/InstrumentDisplayTest.h
    InstrumentWidget/InstrumentWidgetTest.h
    InstrumentWidget/PanelsSurfaceTest.h
    InstrumentWidget/ProjectionSurfaceTest.h
)

set(MOCK_HEADER_DIRS InstrumentWidget)

set(CXXTEST_EXTRA_HEADER_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/InstrumentViewTestInitialization.h")

mtd_add_qt_tests(
  TARGET_NAME MantidQtWidgetsInstrumentWidgetTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ${MOCK_HEADER_DIRS} ../../../../Framework/DataObjects/inc
  LINK_LIBS Python::Python
            ${CORE_MANTIDLIBS}
            Mantid::PythonInterfaceCore
            Mantid::DataObjects
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            gmock
  QT5_LINK_LIBS Qt5::OpenGL
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsInstrumentView MantidQtWidgetsMplCpp MantidQtWidgetsPlotting
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtWidgetsInstrumentWidgetTestQt5)
