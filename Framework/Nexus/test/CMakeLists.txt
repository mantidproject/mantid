if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(NexusTest ${TEST_FILES})

  target_include_directories(NexusTest SYSTEM PRIVATE ${HDF5_INCLUDE_DIRS})
  target_link_libraries(NexusTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        Nexus
                        ${NEXUS_LIBRARIES}
                        ${HDF5_LIBRARIES}
                        ${HDF5_HL_LIBRARIES})
  add_dependencies(FrameworkTests NexusTest)
  add_dependencies(NexusTest StandardTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET NexusTest PROPERTY FOLDER "UnitTests")
endif()
