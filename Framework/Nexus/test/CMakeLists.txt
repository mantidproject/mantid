if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  set(TESTHELPER_SRCS test_helper.cpp)

  cxxtest_add_test(NexusTest ${TEST_FILES})

  target_link_libraries(NexusTest PRIVATE Mantid::Nexus gmock Mantid::Kernel)
  add_dependencies(FrameworkTests NexusTest)
  add_dependencies(NexusTest UnitTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET NexusTest PROPERTY FOLDER "UnitTests")
endif()

# ######### TESTS for napi
set(TEST_PREFIX "NexusNapiTest")

# #### HDF5 The test doesn't want to work under windows
add_executable(napi_test_cpp-hdf5 EXCLUDE_FROM_ALL napi_test_cpp.cpp)
target_link_libraries(napi_test_cpp-hdf5 PRIVATE Mantid::Nexus)
add_test(NAME "${TEST_PREFIX}-Cpp-HDF5-test" COMMAND napi_test_cpp-hdf5)

# #### phony target to drag in all tests

add_custom_target("${TEST_PREFIX}" DEPENDS napi_test_cpp-hdf5)

add_dependencies(FrameworkTests "${TEST_PREFIX}")
