if(CXXTEST_FOUND)

  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable. It will
  # go out of scope at the end of this file so doesn't need un-setting

  cxxtest_add_test(KernelTest ${TEST_FILES})
  target_include_directories(KernelTest SYSTEM PRIVATE ${CXXTEST_INCLUDE_DIR})
  target_link_libraries(KernelTest PRIVATE Mantid::Types Mantid::Kernel Mantid::Json gmock)
  add_framework_test_helpers(KernelTest)
  add_dependencies(FrameworkTests KernelTest)
  # Test data
  add_dependencies(KernelTest UnitTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET KernelTest PROPERTY FOLDER "UnitTests")

  add_custom_command(
    TARGET KernelTest
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/../../Properties/MantidTest.properties
      ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
    COMMAND
      ${CMAKE_COMMAND} ARGS -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/../../Properties/MantidTest.user.properties
      ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
  )

endif()
