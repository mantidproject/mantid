if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ${GMOCK_INCLUDE_DIR}
                      ${GTEST_INCLUDE_DIR})
  check_include_files(stdint.h stdint)
  if(stdint)
    add_definitions(-DHAVE_STDINT_H)
  endif()
  check_include_files(cstdint.hpp boost_stdint)
  if(boost_stdint)
    add_definitions(-DBOOST_CSTDINT_HPP)
  endif()

  # Make the tests
  include_directories(../../Algorithms/inc
                      ../../CurveFitting/inc
                      ../../DataHandling/inc
                      ../../Nexus/inc
                      ../../TestHelpers/inc
                      ../../Muon/inc)

  # This variable is used within the cxxtest_add_test macro to build these
  # helper classes into the test executable. It will go out of scope at the end
  # of this file so doesn't need un-setting
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/FileComparisonHelper.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/MDEventsTestHelper.cpp
      ../../TestHelpers/src/SANSInstrumentCreationHelper.cpp
      ../../TestHelpers/src/ScopedFileHelper.cpp
      ../../TestHelpers/src/TearDownWorld.cpp
      ../../TestHelpers/src/WorkspaceCreationHelper.cpp
      ../../TestHelpers/src/ParallelRunner.cpp
      ../../TestHelpers/src/MuonWorkspaceCreationHelper.cpp
      ../../TestHelpers/src/MuonGroupingXMLHelper.cpp)
  cxxtest_add_test(MuonTest ${TEST_FILES})
  target_link_libraries(MuonTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        Algorithms
                        DataHandling
                        Muon
                        Nexus
                        ${GMOCK_LIBRARIES}
                        ${GTEST_LIBRARIES})
  add_dependencies(MuonTest Crystal CurveFitting)
  add_dependencies(FrameworkTests MuonTest)
  # Test data
  add_dependencies(MuonTest StandardTestData)

  # Add to the 'MuonTest' group in VS
  set_property(TARGET MuonTest PROPERTY FOLDER "UnitTests")
endif()
