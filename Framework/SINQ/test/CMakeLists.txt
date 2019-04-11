if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ${GMOCK_INCLUDE_DIR}
                      ${GTEST_INCLUDE_DIR})

  include_directories(../../CurveFitting/inc
                      ../../MDAlgorithms/inc
                      ../../Nexus/inc
                      ../../NexusCPP/inc
                      ../../TestHelpers/inc)

  set(TESTHELPER_SRCS
      ../../TestHelpers/src/TearDownWorld.cpp
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/WorkspaceCreationHelper.cpp)

  cxxtest_add_test(PSISINQTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(PSISINQTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        API
                        Geometry
                        Kernel
                        SINQ
                        CurveFitting
                        ${MANTIDLIBS}
                        ${GMOCK_LIBRARIES}
                        ${GTEST_LIBRARIES})

  # Test data
  add_dependencies(PSISINQTest StandardTestData)

  add_dependencies(FrameworkTests PSISINQTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET PSISINQTest PROPERTY FOLDER "UnitTests")
endif()
