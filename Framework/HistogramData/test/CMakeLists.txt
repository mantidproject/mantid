if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(HistogramDataTest ${TEST_FILES} ${GMOCK_TEST_FILES})

  target_link_libraries(HistogramDataTest PRIVATE Mantid::HistogramData Mantid::Kernel)

  add_framework_test_helpers(HistogramDataTest)

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(HistogramDataTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(HistogramDataTest PRIVATE gmock gtest)
  endif()
  add_dependencies(FrameworkTests HistogramDataTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET HistogramDataTest PROPERTY FOLDER "UnitTests")
endif()
