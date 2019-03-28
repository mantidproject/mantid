if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # Make the rest of the tests
  include_directories(../../Algorithms/inc ../../DataHandling/inc
                      ../../TestHelpers/inc)
  # This variable is used within the cxxtest_add_test macro to build these
  # helper classes into the test executable. It will go out of scope at the end
  # of this file so doesn't need un-setting
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/TearDownWorld.cpp
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/WorkspaceCreationHelper.cpp)
  cxxtest_add_test(WorkflowAlgorithmsTest ${TEST_FILES})
  target_link_libraries(WorkflowAlgorithmsTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        WorkflowAlgorithms
                        Algorithms
                        DataHandling
                        Nexus)
  add_dependencies(WorkflowAlgorithmsTest CurveFitting)
  add_dependencies(FrameworkTests WorkflowAlgorithmsTest)
  # Test data
  add_dependencies(WorkflowAlgorithmsTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET WorkflowAlgorithmsTest PROPERTY FOLDER "UnitTests")
endif()

# python unit tests
if(PYUNITTEST_FOUND)
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.WorkflowAlgorithms
                      ${TEST_PY_FILES})
endif()
