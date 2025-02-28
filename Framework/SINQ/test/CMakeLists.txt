if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(PSISINQTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(PSISINQTest PRIVATE Mantid::SINQ Mantid::CurveFitting gmock)

  target_include_directories(PSISINQTest PRIVATE Mantid::CurveFitting Mantid::MDAlgorithms Mantid::Nexus Mantid::Nexus)

  add_framework_test_helpers(PSISINQTest)

  # Test data
  add_dependencies(PSISINQTest StandardTestData)

  add_dependencies(FrameworkTests PSISINQTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET PSISINQTest PROPERTY FOLDER "UnitTests")
endif()
