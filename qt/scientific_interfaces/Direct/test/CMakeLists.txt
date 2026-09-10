if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

# Testing
set(TEST_FILES ALFAlgorithmManagerTest.h ALFAnalysisModelTest.h ALFAnalysisPresenterTest.h ALFInstrumentModelTest.h
               ALFInstrumentPresenterTest.h
)

set(TEST_HELPERS ALFAnalysisMocks.h ALFInstrumentMocks.h MockALFAlgorithmManager.h MockALFAlgorithmManagerSubscriber.h
                 MockInstrumentActor.h
)

mtd_add_qt_tests(
  TARGET_NAME MantidQtInterfacesDirectTest
  QT_VERSION ${MANTID_QT_VERSION}
  SRC ${TEST_FILES}
  TEST_HELPER_SRCS ${TEST_HELPERS}
  INCLUDE_DIRS ../
  LINK_LIBS Mantid::API Mantid::DataObjects Mantid::Geometry Mantid::Kernel Mantid::PythonInterfaceCore Python::Python
  MTD_QT_LINK_LIBS MantidScientificInterfacesDirect MantidQtWidgetsCommon MantidQtWidgetsInstrumentView
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidQtInterfacesDirectTestQt${MANTID_QT_VERSION})

if(TARGET GTest::gmock AND TARGET GTest::gtest)

  target_link_libraries(MantidQtInterfacesDirectTestQt${MANTID_QT_VERSION} PRIVATE GTest::gmock GTest::gtest)
else()
  target_link_libraries(MantidQtInterfacesDirectTestQt${MANTID_QT_VERSION} PRIVATE gmock gtest)
endif()
