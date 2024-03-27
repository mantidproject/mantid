set(ALL_TEST_FILES)

add_subdirectory(Reduction)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/InterfacesIndirectTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesIndirectTest
  QT_VERSION 5
  SRC ${ALL_TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/DataObjects/inc ../
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::DataObjects
            gmock
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Mantid::PythonInterfaceCore
            ${PYTHON_LIBRARIES}
  QT5_LINK_LIBS Qt5::OpenGL Qt5::Concurrent
  MTD_QT_LINK_LIBS MantidScientificInterfacesIndirect MantidQtWidgetsCommon MantidQtWidgetsPlotting
                   MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesIndirectTestQt5)
