if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(PSISINQTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(PSISINQTest PRIVATE Mantid::SINQ Mantid::CurveFitting)

  add_framework_test_helpers(PSISINQTest)

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(PSISINQTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(PSISINQTest PRIVATE gmock gtest)
  endif()

  # Test data
  add_dependencies(PSISINQTest UnitTestData)

  add_dependencies(FrameworkTests PSISINQTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET PSISINQTest PROPERTY FOLDER "UnitTests")
endif()
