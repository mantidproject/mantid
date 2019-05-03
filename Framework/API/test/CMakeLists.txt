if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ${GMOCK_INCLUDE_DIR}
                      ${GTEST_INCLUDE_DIR})

  include_directories(../../TestHelpers/inc ../../Nexus/inc
                      ${HDF5_INCLUDE_DIRS})
  # This variable is used within the cxxtest_add_test macro to build these
  # helper classes into the test executable. It will go out of scope at the end
  # of this file so doesn't need un-setting
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/TearDownWorld.cpp
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/BoxControllerDummyIO.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/NexusTestHelper.cpp
      ../../TestHelpers/src/FakeObjects.cpp
      ../../TestHelpers/src/ParallelRunner.cpp)
  cxxtest_add_test(APITest ${TEST_FILES} ${GMOCK_TEST_FILES})
  if(WIN32)
    set(BCRYPT bcrypt)
  endif()
  target_link_libraries(APITest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        ${BCRYPT}
                        Types
                        API
                        Nexus
                        ${JSONCPP_LIBRARIES}
                        ${NEXUS_LIBRARIES}
                        ${MUPARSER_LIBRARIES}
                        ${GMOCK_LIBRARIES}
                        ${GTEST_LIBRARIES})

  add_dependencies(FrameworkTests APITest)
  # Test data
  add_dependencies(APITest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET APITest PROPERTY FOLDER "UnitTests")
endif()
