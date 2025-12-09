if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable. It
  # will go out of scope at the end of this file so doesn't need un-setting

  cxxtest_add_test(IndexingTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(IndexingTest PRIVATE Mantid::Types Mantid::Indexing gmock gtest)

  add_framework_test_helpers(IndexingTest)
  add_dependencies(FrameworkTests IndexingTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET IndexingTest PROPERTY FOLDER "UnitTests")
endif()
