if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ${GMOCK_INCLUDE_DIR}
                      ${GTEST_INCLUDE_DIR})

  include_directories(../../DataHandling/inc ../../TestHelpers/inc)
  # This variable is used within the cxxtest_add_test macro to build these
  # helper classes into the test executable. It will go out of scope at the end
  # of this file so doesn't need un-setting
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/FunctionCreationHelper.cpp
      ../../TestHelpers/src/MultiDomainFunctionHelper.cpp
      ../../TestHelpers/src/StartFrameworkManager.cpp
      ../../TestHelpers/src/TearDownWorld.cpp
      ../../TestHelpers/src/WorkspaceCreationHelper.cpp)
  cxxtest_add_test(CurveFittingTest ${TEST_FILES})
  target_link_libraries(CurveFittingTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        CurveFitting
                        DataHandling
                        ${GSL_LIBRARIES}
                        ${GMOCK_LIBRARIES}
                        ${GTEST_LIBRARIES})
  add_dependencies(CurveFittingTest Algorithms)
  add_dependencies(FrameworkTests CurveFittingTest)
  # Test data
  add_dependencies(CurveFittingTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET CurveFittingTest PROPERTY FOLDER "UnitTests")
endif()
