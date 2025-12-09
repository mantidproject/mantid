if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  set(TESTHELPER_SRCS test_helper.cpp)

  cxxtest_add_test(NexusTest ${TEST_FILES})

  target_link_libraries(NexusTest PRIVATE Mantid::Nexus gmock gtest Mantid::Kernel)
  add_dependencies(FrameworkTests NexusTest)
  add_dependencies(NexusTest UnitTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET NexusTest PROPERTY FOLDER "UnitTests")
endif()
