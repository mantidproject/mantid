if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  include_directories(../)

  cxxtest_add_test(ScriptRepositoryTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(ScriptRepositoryTest PRIVATE Mantid::Types Mantid::ScriptRepository gmock gtest)

  add_dependencies(FrameworkTests ScriptRepositoryTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET ScriptRepositoryTest PROPERTY FOLDER "UnitTests")
endif()
