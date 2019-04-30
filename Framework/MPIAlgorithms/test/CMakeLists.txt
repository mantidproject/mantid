if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  include_directories(../../TestHelpers/inc)
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/TearDownWorld.cpp
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/WorkspaceCreationHelper.cpp)

  cxxtest_add_test(MPIAlgorithmsTest ${TEST_FILES})
  target_link_libraries(MPIAlgorithmsTest MPIAlgorithms)
  add_dependencies(FrameworkTests MPIAlgorithmsTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET MPIAlgorithmsTest PROPERTY FOLDER "UnitTests")
endif()
