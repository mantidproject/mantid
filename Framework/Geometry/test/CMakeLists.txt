if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable. It will
  # go out of scope at the end of this file so doesn't need un-setting

  cxxtest_add_test(GeometryTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_compile_definitions(GeometryTest PRIVATE -D_SILENCE_FPOS_SEEKPOS_DEPRECATION_WARNING)
  target_link_libraries(
    GeometryTest PRIVATE Mantid::Geometry Mantid::Beamline Mantid::Types Mantid::Kernel ${OpenGL_LIBRARIES}
  )

  target_precompile_headers(GeometryTest PRIVATE <cxxtest/WrappedTestSuite.h> <set> <string> <vector>)

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(GeometryTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(GeometryTest PRIVATE gmock gtest)
  endif()

  add_framework_test_helpers(GeometryTest)
  add_dependencies(FrameworkTests GeometryTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET GeometryTest PROPERTY FOLDER "UnitTests")
endif()
