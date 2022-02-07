if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(TypesTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(TypesTest PRIVATE Mantid::Types gmock)

  add_dependencies(FrameworkTests TypesTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET TypesTest PROPERTY FOLDER "UnitTests")
endif()
