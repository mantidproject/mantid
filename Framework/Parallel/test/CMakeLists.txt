if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ../../TestHelpers/inc)

  # This variable is used within the cxxtest_add_test macro to build these
  # helper classes into the test executable. It will go out of scope at the end
  # of this file so doesn't need un-setting
  set(TESTHELPER_SRCS ../../TestHelpers/src/ParallelRunner.cpp)

  cxxtest_add_test(ParallelTest ${TEST_FILES} ${GMOCK_TEST_FILES})

  target_link_libraries(ParallelTest
                        PRIVATE
                        Mantid::Parallel
                        Mantid::Types
                        gmock)

  add_dependencies(FrameworkTests ParallelTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET ParallelTest PROPERTY FOLDER "UnitTests")
endif()
