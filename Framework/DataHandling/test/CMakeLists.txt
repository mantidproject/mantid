if(CXXTEST_FOUND)
  include_directories(SYSTEM
				      ${CXXTEST_INCLUDE_DIR})

  include_directories(../../TestHelpers/inc)
  # This variable is used within the cxxtest_add_test macro to build this helper
  # class into the test executable. It will go out of scope at the end of this
  # file so doesn't need un-setting
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/ComponentCreationHelper.cpp
      ../../TestHelpers/src/FileComparisonHelper.cpp
      ../../TestHelpers/src/InstrumentCreationHelper.cpp
      ../../TestHelpers/src/ScopedFileHelper.cpp
      ../../TestHelpers/src/StartFrameworkManager.cpp
      ../../TestHelpers/src/TearDownWorld.cpp
      ../../TestHelpers/src/WorkspaceCreationHelper.cpp
      ../../TestHelpers/src/NexusTestHelper.cpp
      ../../TestHelpers/src/ONCatHelper.cpp
      ../../TestHelpers/src/ParallelRunner.cpp
      NXcanSASTestHelper.cpp
	  ../../TestHelpers/src/FileResource.cpp)

  cxxtest_add_test(DataHandlingTest ${TEST_FILES})

  target_link_libraries(DataHandlingTest
                        PRIVATE
                        Mantid::Kernel
                        Mantid::Catalog
                        Mantid::DataHandling
                        Mantid::Nexus
                        Mantid::NexusGeometry
                        Mantid::HistogramData
                        Boost::filesystem
                        gmock)

  if(ENABLE_LIB3MF)
    target_link_libraries(DataHandlingTest LINK_PRIVATE
                          ${LIB3MF_LIBRARIES})
    if (CONDA_BUILD)
      target_include_directories(DataHandlingTest PRIVATE ${LIB3MF_INCLUDE_DIR})
    else()
      target_include_directories(DataHandlingTest PRIVATE ${LIB3MF_INCLUDE_DIR}/lib3mf)
    endif()
  endif()

  add_dependencies(DataHandlingTest Algorithms MDAlgorithms)
  add_dependencies(FrameworkTests DataHandlingTest)
  # Test data
  add_dependencies(DataHandlingTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET DataHandlingTest PROPERTY FOLDER "UnitTests")
endif()
