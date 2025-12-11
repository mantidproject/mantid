if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  set(TESTHELPER_SRCS test_helper.cpp)

  cxxtest_add_test(LegacyNexusTest ${TEST_FILES})

  target_link_libraries(LegacyNexusTest PRIVATE Mantid::LegacyNexus)
  add_framework_test_helpers(LegacyNexusTest)

  if(TARGET GTest::gmock_main)
    target_link_libraries(LegacyNexusTest PRIVATE GTest::gmock_main)
  elseif(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(LegacyNexusTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(LegacyNexusTest PRIVATE gmock gtest)
  endif()
  add_dependencies(FrameworkTests LegacyNexusTest)
  add_dependencies(LegacyNexusTest UnitTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET LegacyNexusTest PROPERTY FOLDER "UnitTests")
endif()
