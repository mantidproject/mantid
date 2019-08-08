if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ${GMOCK_INCLUDE_DIR}
                      ${GTEST_INCLUDE_DIR}
                      ${Boost_INCLUDE_DIRS}
                      ../../TestHelpers/inc)

  set(TESTHELPER_SRCS ../../TestHelpers/src/NexusGeometryTestHelpers.cpp ../../TestHelpers/src/ComponentCreationHelper.cpp)

  cxxtest_add_test(NexusGeometryTest ${TEST_FILES})

  target_include_directories(NexusGeometryTest SYSTEM
                             PRIVATE ${HDF5_INCLUDE_DIRS})
  target_link_libraries(NexusGeometryTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        NexusGeometry
                        Geometry
                        Nexus
                        API
						Types
						Kernel
                        ${NEXUS_LIBRARIES}
                        ${HDF5_LIBRARIES}
                        ${HDF5_HL_LIBRARIES}
                        ${GMOCK_LIBRARIES})
  add_dependencies(NexusGeometryTest Geometry)
  add_dependencies(FrameworkTests NexusGeometryTest)
  add_dependencies(NexusGeometryTest StandardTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET NexusGeometryTest PROPERTY FOLDER "UnitTests")
endif()
