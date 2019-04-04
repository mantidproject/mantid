if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  include_directories(../../TestHelpers/inc)
  # This variable is used within the cxxtest_add_test macro to build this helper
  # class into the test executable. It will go out of scope at the end of this
  # file so doesn't need un-setting
  set(TESTHELPER_SRCS ICatTestHelper.cpp
      ../../TestHelpers/src/TearDownWorld.cpp)

  # The actual test suite
  cxxtest_add_test(ICatTest ${TEST_FILES})
  target_link_libraries(ICatTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        ICat)
  add_dependencies(FrameworkTests ICatTest)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET ICatTest PROPERTY FOLDER "UnitTests")
endif()
