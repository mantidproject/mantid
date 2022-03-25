if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(HistogramDataTest ${TEST_FILES} ${GMOCK_TEST_FILES})

  target_link_libraries(HistogramDataTest PRIVATE Mantid::HistogramData Mantid::Kernel gmock)

  add_framework_test_helpers(HistogramDataTest)
  add_dependencies(FrameworkTests HistogramDataTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET HistogramDataTest PROPERTY FOLDER "UnitTests")
endif()
