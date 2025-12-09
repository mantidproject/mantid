if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  set(TESTHELPER_SRCS test_helper.cpp)

  cxxtest_add_test(LegacyNexusTest ${TEST_FILES})

  target_link_libraries(LegacyNexusTest PRIVATE Mantid::LegacyNexus gmock gtest)
  add_framework_test_helpers(LegacyNexusTest)
  add_dependencies(FrameworkTests LegacyNexusTest)
  add_dependencies(LegacyNexusTest UnitTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET LegacyNexusTest PROPERTY FOLDER "UnitTests")
endif()
