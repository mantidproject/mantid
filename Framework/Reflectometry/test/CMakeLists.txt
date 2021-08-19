if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR})

  include_directories(../../Algorithms/inc
                      ../../TestHelpers/inc
                      ../../Reflectometry/inc)

  # This variable is used within the cxxtest_add_test macro to build these
  # helper classes into the test executable. It will go out of scope at the end
  # of this file so doesn't need un-setting
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/ReflectometryHelper.cpp
      ../../TestHelpers/src/WorkspaceCreationHelper.cpp)

  cxxtest_add_test(ReflectometryTest ${TEST_FILES})
  target_link_libraries(ReflectometryTest
                        PRIVATE
                        Mantid::Algorithms
                        Mantid::Reflectometry
                        gmock
)
  add_dependencies(FrameworkTests ReflectometryTest)
  # Test data
  add_dependencies(ReflectometryTest StandardTestData)

  # Add to the 'ReflectometryTest' group in VS
  set_property(TARGET ReflectometryTest PROPERTY FOLDER "UnitTests")
endif()

# python unit tests
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Reflectometry
                    ${TEST_PY_FILES})

