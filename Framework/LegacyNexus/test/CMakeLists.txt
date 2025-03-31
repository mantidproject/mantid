if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  set(TESTHELPER_SRCS test_helper.cpp)

  cxxtest_add_test(LegacyNexusTest ${TEST_FILES})

  # target_link_libraries(LegacyNexusTest PRIVATE Mantid::LegacyNexus gmock)
  add_framework_test_helpers(LegacyNexusTest)
  add_dependencies(FrameworkTests LegacyNexusTest)
  add_dependencies(LegacyNexusTest UnitTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET LegacyNexusTest PROPERTY FOLDER "UnitTests")
endif()

# ######### TESTS for napi
set(TEST_PREFIX "LegacyNexusNapiTest")

# #### HDF5 The test doesn't want to work under windows
add_executable(legacy_napi_test_hdf5 EXCLUDE_FROM_ALL napi_test.cpp napi_test_util.cpp napi_test_util.h)
target_link_libraries(legacy_napi_test_hdf5 PRIVATE Mantid::LegacyNexus)
add_test(NAME "${TEST_PREFIX}-C-HDF5-test" COMMAND legacy_napi_test_hdf5)

add_executable(legacy_napi_test_cpp-hdf5 EXCLUDE_FROM_ALL napi_test_cpp.cpp napi_test_util.cpp napi_test_util.h)
target_link_libraries(legacy_napi_test_cpp-hdf5 PRIVATE Mantid::LegacyNexus)
add_test(NAME "${TEST_PREFIX}-Cpp-HDF5-test" COMMAND legacy_napi_test_cpp-hdf5)

# #### test for unlimited dimensions
add_executable(legacy_napi_test_nxunlimited EXCLUDE_FROM_ALL test_nxunlimited.cpp)
target_link_libraries(legacy_napi_test_nxunlimited PRIVATE Mantid::LegacyNexus)
add_test(NAME "${TEST_PREFIX}-C-test-nxunlimited" COMMAND legacy_napi_test_nxunlimited)

# #### phony target to drag in all tests

add_custom_target("${TEST_PREFIX}" DEPENDS legacy_napi_test_hdf5 legacy_napi_test_cpp-hdf5 legacy_napi_test_nxunlimited)

add_dependencies(FrameworkTests "${TEST_PREFIX}")
