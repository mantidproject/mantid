if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS})

  include_directories(../../DataHandling/inc ../../CurveFitting/inc)
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable. It
  # will go out of scope at the end of this file so doesn't need un-setting
  cxxtest_add_test(MDAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(
    MDAlgorithmsTest PRIVATE Mantid::DataHandling Mantid::CurveFitting Mantid::MDAlgorithms Mantid::Nexus
                             Mantid::Kernel gmock
  )

  target_include_directories(MDAlgorithmsTest PRIVATE CurveFitting DataHandling)

  add_framework_test_helpers(MDAlgorithmsTest)
  add_dependencies(MDAlgorithmsTest DataHandling Algorithms CurveFitting)
  add_dependencies(FrameworkTests MDAlgorithmsTest)
  # Test data add_dependencies(MDAlgorithmsTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET MDAlgorithmsTest PROPERTY FOLDER "UnitTests")
endif()
