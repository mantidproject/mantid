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

  # Generate the MultiplyTest.h and DivideTest.h
  set(MULTIPLYDIVIDETEST_CLASS MultiplyTest)
  set(MULTIPLYDIVIDETEST_DO_DIVIDE false)
  configure_file(MultiplyDivideTest.in.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/MultiplyTest.h)
  set(MULTIPLYDIVIDETEST_CLASS DivideTest)
  set(MULTIPLYDIVIDETEST_DO_DIVIDE true)
  configure_file(MultiplyDivideTest.in.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/DivideTest.h)

  # Generate the PlusTest.h and MinusTest.h
  set(PLUSMINUSTEST_CLASS PlusTest)
  set(PLUSMINUSTEST_DO_PLUS true)
  configure_file(PlusMinusTest.in.h ${CMAKE_CURRENT_SOURCE_DIR}/PlusTest.h)
  set(PLUSMINUSTEST_CLASS MinusTest)
  set(PLUSMINUSTEST_DO_PLUS false)
  configure_file(PlusMinusTest.in.h ${CMAKE_CURRENT_SOURCE_DIR}/MinusTest.h)

  # Make the rest of the tests
  include_directories(../../CurveFitting/inc
                      ../../DataHandling/inc
                      ../../Nexus/inc
                      ../../TestHelpers/inc)
  # This variable is used within the cxxtest_add_test macro to build these
  # helper classes into the test executable. It will go out of scope at the end
  # of this file so doesn't need un-setting
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/FileComparisonHelper.cpp
      ../../TestHelpers/src/IndirectFitDataCreationHelper.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/MDEventsTestHelper.cpp
      ../../TestHelpers/src/MuonWorkspaceCreationHelper.cpp
      ../../TestHelpers/src/SANSInstrumentCreationHelper.cpp
      ../../TestHelpers/src/ScopedFileHelper.cpp
      ../../TestHelpers/src/TearDownWorld.cpp
      ../../TestHelpers/src/WorkspaceCreationHelper.cpp
      ../../TestHelpers/src/ParallelRunner.cpp
      ../../TestHelpers/src/ReflectometryHelper.cpp)
  cxxtest_add_test(AlgorithmsTest ${TEST_FILES})
  target_link_libraries(AlgorithmsTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        Algorithms
                        DataHandling
                        Nexus
                        ${GMOCK_LIBRARIES}
                        ${GTEST_LIBRARIES})
  add_dependencies(AlgorithmsTest Crystal CurveFitting)
  add_dependencies(FrameworkTests AlgorithmsTest)
  # Test data
  add_dependencies(AlgorithmsTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET AlgorithmsTest PROPERTY FOLDER "UnitTests")
endif()

# python unit tests
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.algorithms
  ${TEST_PY_FILES})
