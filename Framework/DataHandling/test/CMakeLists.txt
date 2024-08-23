if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable. It will
  # go out of scope at the end of this file so doesn't need un-setting
  set(TESTHELPER_SRCS NXcanSASTestHelper.cpp)

  cxxtest_add_test(DataHandlingTest ${TEST_FILES})

  target_link_libraries(
    DataHandlingTest
    PRIVATE Mantid::Kernel
            Mantid::Catalog
            Mantid::DataHandling
            Mantid::Nexus
            Mantid::NexusGeometry
            Mantid::HistogramData
            Boost::filesystem
            gmock
  )

  if(ENABLE_LIB3MF)
    target_link_libraries(DataHandlingTest LINK_PRIVATE ${LIB3MF_LIBRARIES})
    target_include_directories(DataHandlingTest PRIVATE ${LIB3MF_INCLUDE_DIR})
  endif()
  add_framework_test_helpers(DataHandlingTest)
  add_dependencies(DataHandlingTest Algorithms MDAlgorithms)
  add_dependencies(FrameworkTests DataHandlingTest)
  # Test data
  add_dependencies(DataHandlingTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET DataHandlingTest PROPERTY FOLDER "UnitTests")
endif()
