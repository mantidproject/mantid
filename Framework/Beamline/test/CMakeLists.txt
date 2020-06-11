if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ../../TestHelpers/inc)

  cxxtest_add_test(BeamlineTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_include_directories(BeamlineTest SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(BeamlineTest
                        PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        Beamline
			Kernel
                        ${Boost_LIBRARIES}
                        gmock
  )

  if(OpenMP_CXX_FOUND)
    # Access to config service is required
    target_link_libraries(BeamlineTest PRIVATE Kernel)
  endif()

  add_dependencies(FrameworkTests BeamlineTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET BeamlineTest PROPERTY FOLDER "UnitTests")
endif()
