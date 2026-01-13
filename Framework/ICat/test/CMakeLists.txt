if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable. It will
  # go out of scope at the end of this file so doesn't need un-setting
  set(TESTHELPER_SRCS FakeCatalog.cpp ICatTestHelper.cpp)

  # The actual test suite
  cxxtest_add_test(ICatTest ${TEST_FILES})
  target_link_libraries(ICatTest PRIVATE Mantid::ICat Mantid::API Mantid::DataObjects)

  target_precompile_headers(ICatTest PRIVATE <cxxtest/WrappedTestSuite.h> <set> <string> <vector>)

  add_framework_test_helpers(ICatTest)

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(ICatTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(ICatTest PRIVATE gmock gtest)
  endif()
  add_dependencies(FrameworkTests ICatTest)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET ICatTest PROPERTY FOLDER "UnitTests")
endif()
