if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ../../TestHelpers/inc)

  set(TESTHELPER_SRCS ../../TestHelpers/src/NexusGeometryTestHelpers.cpp
                      ../../TestHelpers/src/JSONGeometryParserTestHelper.cpp
                      ../../TestHelpers/src/ComponentCreationHelper.cpp
					  ../../TestHelpers/src/FileResource.cpp)

  cxxtest_add_test(NexusGeometryTest ${TEST_FILES})

  target_link_libraries(NexusGeometryTest
                        PRIVATE
                        Mantid::NexusGeometry
                        Mantid::Types
                        Mantid::Kernel
                        Boost::filesystem
                        gmock)

  add_dependencies(NexusGeometryTest Geometry)
  add_dependencies(FrameworkTests NexusGeometryTest)
  add_dependencies(NexusGeometryTest StandardTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET NexusGeometryTest PROPERTY FOLDER "UnitTests")
endif()
