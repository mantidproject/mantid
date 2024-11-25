if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(NexusTest ${TEST_FILES})

  target_link_libraries(NexusTest PRIVATE Mantid::API Mantid::Nexus gmock)
  add_framework_test_helpers(NexusTest)
  add_dependencies(FrameworkTests NexusTest)
  add_dependencies(NexusTest StandardTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET NexusTest PROPERTY FOLDER "UnitTests")
endif()
