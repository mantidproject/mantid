if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable. It will
  # go out of scope at the end of this file so doesn't need un-setting
  cxxtest_add_test(DataObjectsTest ${TEST_FILES})
  target_link_libraries(DataObjectsTest PRIVATE Mantid::DataObjects Mantid::Kernel Nexus::nexuscpp gmock)

  add_framework_test_helpers(DataObjectsTest)
  # Specify implicit dependency, but don't link to it
  add_dependencies(FrameworkTests DataObjectsTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET DataObjectsTest PROPERTY FOLDER "UnitTests")
endif()
