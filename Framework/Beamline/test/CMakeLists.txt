if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(BeamlineTest ${TEST_FILES} ${GMOCK_TEST_FILES})

  target_link_libraries(BeamlineTest PRIVATE Mantid::Beamline Mantid::Kernel gmock)

  add_framework_test_helpers(BeamlineTest)

  add_dependencies(FrameworkTests BeamlineTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET BeamlineTest PROPERTY FOLDER "UnitTests")
endif()
