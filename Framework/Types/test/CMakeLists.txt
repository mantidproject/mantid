if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(TypesTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(TypesTest PRIVATE Mantid::Types)

  add_dependencies(FrameworkTests TypesTest)

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(TypesTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(TypesTest PRIVATE gmock gtest)
  endif()
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET TypesTest PROPERTY FOLDER "UnitTests")
endif()
