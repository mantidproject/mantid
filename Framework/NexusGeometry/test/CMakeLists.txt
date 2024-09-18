if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(NexusGeometryTest ${TEST_FILES})

  target_link_libraries(NexusGeometryTest PRIVATE Mantid::NexusGeometry Mantid::Types Mantid::Kernel gmock)

  add_framework_test_helpers(NexusGeometryTest)
  add_dependencies(NexusGeometryTest Geometry)
  add_dependencies(FrameworkTests NexusGeometryTest)
  add_dependencies(NexusGeometryTest StandardTestData)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET NexusGeometryTest PROPERTY FOLDER "UnitTests")
endif()
