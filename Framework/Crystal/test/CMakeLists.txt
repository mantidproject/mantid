if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable. It
  # will go out of scope at the end of this file so doesn't need un-setting

  cxxtest_add_test(CrystalTest ${TEST_FILES})
  target_link_libraries(
    CrystalTest PRIVATE Mantid::Crystal Mantid::DataHandling Mantid::MDAlgorithms Mantid::Nexus Boost::filesystem gmock
  )
  add_framework_test_helpers(CrystalTest)
  add_dependencies(CrystalTest Algorithms CurveFitting)
  add_dependencies(FrameworkTests CrystalTest)
  # Test data
  add_dependencies(CrystalTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET CrystalTest PROPERTY FOLDER "UnitTests")
endif()
