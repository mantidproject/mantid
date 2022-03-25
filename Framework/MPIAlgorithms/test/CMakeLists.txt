if(CXXTEST_FOUND)

  cxxtest_add_test(MPIAlgorithmsTest ${TEST_FILES})
  target_link_libraries(MPIAlgorithmsTest PRIVATE Mantid::MPIAlgorithms)

  add_framework_test_helpers(MPIAlgorithmsTest)
  add_dependencies(FrameworkTests MPIAlgorithmsTest)

  target_include_directories(MPIAlgorithmsTest SYSTEM ${CXXTEST_INCLUDE_DIR})
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET MPIAlgorithmsTest PROPERTY FOLDER "UnitTests")
endif()
