if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

set(ALL_TEST_FILES)
set(ALL_TEST_HELPERS)

add_subdirectory(BayesFitting)
add_subdirectory(Processor)
add_subdirectory(QENSFitting)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/InterfacesInelasticTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesInelasticTest
  QT_VERSION 5
  SRC ${ALL_TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/DataObjects/inc ../
  TEST_HELPER_SRCS ${ALL_TEST_HELPERS}
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::DataObjects
            gmock
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Mantid::PythonInterfaceCore
            ${PYTHON_LIBRARIES}
  QT5_LINK_LIBS Qt5::OpenGL
  MTD_QT_LINK_LIBS MantidScientificInterfacesInelastic MantidQtWidgetsCommon MantidQtWidgetsPlotting
                   MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesInelasticTestQt5)
