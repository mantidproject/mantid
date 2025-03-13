if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(NexusTest ${TEST_FILES})

  target_link_libraries(NexusTest PRIVATE Mantid::API Mantid::Nexus gmock)
  add_framework_test_helpers(NexusTest)
  add_dependencies(FrameworkTests NexusTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET NexusTest PROPERTY FOLDER "UnitTests")
endif()

# ######### TESTS for napi
set(TEST_PREFIX "NexusNapiTest")

# #### HDF5 The test doesn't want to work under windows
add_executable(napi_test_hdf5 EXCLUDE_FROM_ALL napi_test.cpp napi_test_util.cpp napi_test_util.h)
target_link_libraries(napi_test_hdf5 PRIVATE Mantid::Nexus)
add_test(NAME "${TEST_PREFIX}-C-HDF5-test" COMMAND napi_test_hdf5)

add_executable(napi_test_cpp-hdf5 EXCLUDE_FROM_ALL napi_test_cpp.cpp napi_test_util.cpp napi_test_util.h)
target_link_libraries(napi_test_cpp-hdf5 PRIVATE Mantid::Nexus)
add_test(NAME "${TEST_PREFIX}-Cpp-HDF5-test" COMMAND napi_test_cpp-hdf5)

# #### test for unlimited dimensions
add_executable(napi_test_nxunlimited EXCLUDE_FROM_ALL test_nxunlimited.cpp)
target_link_libraries(napi_test_nxunlimited PRIVATE Mantid::Nexus)
add_test(NAME "${TEST_PREFIX}-C-test-nxunlimited" COMMAND napi_test_nxunlimited)

# #### leak tests

add_executable(napi_leak_test1 EXCLUDE_FROM_ALL leak_test1.cpp napi_test_util.cpp napi_test_util.h)
target_link_libraries(napi_leak_test1 PRIVATE Mantid::Nexus)
add_test(NAME "${TEST_PREFIX}-C-leak-test-1" COMMAND napi_leak_test1)

add_executable(napi_leak_test2 EXCLUDE_FROM_ALL leak_test2.cpp)
target_link_libraries(napi_leak_test2 PRIVATE Mantid::Nexus)
add_test(NAME "${TEST_PREFIX}-Cpp-leak-test-2" COMMAND napi_leak_test2)

add_executable(napi_leak_test3 EXCLUDE_FROM_ALL leak_test3.cpp)
target_link_libraries(napi_leak_test3 PRIVATE Mantid::Nexus)
add_test(NAME "${TEST_PREFIX}-Cpp-leak-test-3" COMMAND napi_leak_test3)

# #### phony target to drag in all tests
add_custom_target(
  "${TEST_PREFIX}" DEPENDS napi_test_hdf5 napi_test_cpp-hdf5 napi_test_nxunlimited napi_leak_test1 napi_leak_test2
                           napi_leak_test3
)

add_dependencies(FrameworkTests "${TEST_PREFIX}")
