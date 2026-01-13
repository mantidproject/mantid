if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable. It
  # will go out of scope at the end of this file so doesn't need un-setting
  cxxtest_add_test(CurveFittingTest ${TEST_FILES})
  target_link_libraries(CurveFittingTest PRIVATE Mantid::CurveFitting Mantid::DataHandling)

  target_precompile_headers(
    CurveFittingTest
    PRIVATE
    <cxxtest/WrappedTestSuite.h>
    <MantidAPI/Algorithm.h>
    <MantidAPI/IFunction.h>
    <MantidAPI/IPeakFunction.h>
    <MantidKernel/System.h>
    <set>
    <string>
    <vector>
  )

  add_framework_test_helpers(CurveFittingTest)

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(CurveFittingTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(CurveFittingTest PRIVATE gmock gtest)
  endif()

  add_dependencies(CurveFittingTest Algorithms)
  add_dependencies(FrameworkTests CurveFittingTest)
  # Test data
  add_dependencies(CurveFittingTest UnitTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET CurveFittingTest PROPERTY FOLDER "UnitTests")
endif()
