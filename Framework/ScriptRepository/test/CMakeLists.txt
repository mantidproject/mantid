if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  include_directories(../)

  cxxtest_add_test(ScriptRepositoryTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(ScriptRepositoryTest PRIVATE Mantid::Types Mantid::ScriptRepository)

  add_dependencies(FrameworkTests ScriptRepositoryTest)

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(ScriptRepositoryTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(ScriptRepositoryTest PRIVATE gmock gtest)
  endif()
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET ScriptRepositoryTest PROPERTY FOLDER "UnitTests")
endif()
