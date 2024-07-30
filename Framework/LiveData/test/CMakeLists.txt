if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable. It will
  # go out of scope at the end of this file so doesn't need un-setting
  set(TESTHELPER_SRCS KafkaTesting.h KafkaTestThreadHelper.h TestDataListener.cpp TestGroupDataListener.cpp)

  cxxtest_add_test(LiveDataTest ${TEST_FILES})
  target_link_libraries(LiveDataTest PRIVATE Mantid::LiveData gmock)
  target_include_directories(LiveDataTest PRIVATE ../src/)
  add_framework_test_helpers(LiveDataTest)
  add_dependencies(LiveDataTest DataHandling Algorithms MDAlgorithms)
  add_dependencies(FrameworkTests LiveDataTest)
  # Test data
  add_dependencies(LiveDataTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET LiveDataTest PROPERTY FOLDER "UnitTests")
endif()
