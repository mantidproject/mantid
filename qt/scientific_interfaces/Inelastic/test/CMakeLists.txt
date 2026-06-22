if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

set(ALL_TEST_FILES)
set(ALL_TEST_HELPERS)

add_subdirectory(BayesFitting)
add_subdirectory(Corrections)
add_subdirectory(Processor)
add_subdirectory(QENSFitting)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/InterfacesInelasticTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesInelasticTest
  QT_VERSION ${MANTID_QT_VERSION}
  SRC ${ALL_TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/DataObjects/inc ../
  TEST_HELPER_SRCS ${ALL_TEST_HELPERS}
  LINK_LIBS ${CORE_MANTIDLIBS} Mantid::DataObjects ${POCO_LIBRARIES} ${Boost_LIBRARIES} Mantid::PythonInterfaceCore
            ${PYTHON_LIBRARIES}
  QT5_LINK_LIBS Qt5::OpenGL QT6_LINK_LIBS Qt6::OpenGL Qt6::OpenGLWidgets
  MTD_QT_LINK_LIBS MantidScientificInterfacesInelastic MantidQtWidgetsCommon MantidQtWidgetsPlotting
                   MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesInelasticTestQt${MANTID_QT_VERSION})

if(TARGET GTest::gmock AND TARGET GTest::gtest)
  target_link_libraries(MantidQtInterfacesInelasticTestQt${MANTID_QT_VERSION} PRIVATE GTest::gmock GTest::gtest)
else()
  target_link_libraries(MantidQtInterfacesInelasticTestQt${MANTID_QT_VERSION} PRIVATE gmock gtest)
endif()
