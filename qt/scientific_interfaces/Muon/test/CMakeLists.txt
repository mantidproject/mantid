if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

# Testing
set(TEST_FILES
    ALCBaselineModellingModelTest.h
    ALCBaselineModellingPresenterTest.h
    ALCDataLoadingPresenterTest.h
    ALCLatestFileFinderTest.h
    ALCPeakFittingModelTest.h
    ALCPeakFittingPresenterTest.h
    MuonAnalysisHelperTest.h
)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/InterfacesMuonTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesMuonTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/DataObjects/inc ../
  LINK_LIBS ${CORE_MANTIDLIBS} Mantid::DataObjects ${POCO_LIBRARIES} ${Boost_LIBRARIES} Mantid::PythonInterfaceCore
            Python::Python
  QT5_LINK_LIBS Qt5::Test Qt5::OpenGL
  MTD_QT_LINK_LIBS MantidScientificInterfacesMuon MantidQtWidgetsCommon MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesMuonTestQt5)

if(TARGET GTest::gmock_main)
  target_link_libraries(MantidQtInterfacesMuonTestQt5 PRIVATE GTest::gmock_main)
elseif(TARGET GTest::gmock AND TARGET GTest::gtest)
  target_link_libraries(MantidQtInterfacesMuonTestQt5 PRIVATE GTest::gmock GTest::gtest)
else()
  target_link_libraries(MantidQtInterfacesMuonTestQt5 PRIVATE gmock gtest)
endif()
